/*
 * Simple RDMA CM client: connects to the server and exchanges one SEND/RECV
 * message over a Reliable Connected (RC) queue pair.
 *
 * Build: gcc -O2 -Wall -o client client.c -libverbs -lrdmacm
 * Run:   ./client <server-ip> [port]  (default port: 18515)
 */
#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>

#define DEFAULT_PORT "18515"
#define MSG_SIZE 256
#define WR_SEND 1
#define WR_RECV 2

static struct rdma_event_channel *channel;
static struct rdma_cm_id *id;
static struct ibv_pd *pd;
static struct ibv_cq *cq;
static struct ibv_qp *qp;
static struct ibv_mr *mr;
static char buf[2][MSG_SIZE]; /* buf[0]: send, buf[1]: recv */

static void die(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

static void die_verbs(const char *msg, int ret)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(-ret));
	exit(EXIT_FAILURE);
}

static void wait_event(enum rdma_cm_event_type expected)
{
	struct rdma_cm_event *ev;
	enum rdma_cm_event_type got;
	const char *name;

	for (;;) {
		if (rdma_get_cm_event(channel, &ev)) {
			if (errno == EINTR)
				continue;
			die("rdma_get_cm_event");
		}
		got = ev->event;
		name = rdma_event_str(got);
		if (rdma_ack_cm_event(ev))
			die("rdma_ack_cm_event");
		if (got == expected)
			return;
		fprintf(stderr, "unexpected CM event %s while waiting for %s\n",
			name, rdma_event_str(expected));
		exit(EXIT_FAILURE);
	}
}

static void setup_qp(void)
{
	struct ibv_qp_init_attr attr;

	memset(&attr, 0, sizeof(attr));
	pd = ibv_alloc_pd(id->verbs);
	if (!pd)
		die("ibv_alloc_pd");

	cq = ibv_create_cq(id->verbs, 8, NULL, NULL, 0);
	if (!cq)
		die("ibv_create_cq");

	attr.send_cq = cq;
	attr.recv_cq = cq;
	attr.qp_type = IBV_QPT_RC;
	attr.cap.max_send_wr = 4;
	attr.cap.max_recv_wr = 4;
	attr.cap.max_send_sge = 1;
	attr.cap.max_recv_sge = 1;
	if (rdma_create_qp(id, pd, &attr))
		die("rdma_create_qp");
	qp = id->qp;

	mr = ibv_reg_mr(pd, buf, sizeof(buf), IBV_ACCESS_LOCAL_WRITE);
	if (!mr)
		die("ibv_reg_mr");
}

static void post_recv(void)
{
	struct ibv_sge sge = {
		.addr = (uintptr_t)buf[1],
		.length = MSG_SIZE,
		.lkey = mr->lkey,
	};
	struct ibv_recv_wr wr = {
		.wr_id = WR_RECV,
		.sg_list = &sge,
		.num_sge = 1,
	};
	struct ibv_recv_wr *bad;
	int ret = ibv_post_recv(qp, &wr, &bad);

	if (ret)
		die_verbs("ibv_post_recv", ret);
}

static void wait_cq(uint64_t wr_id)
{
	struct ibv_wc wc;

	for (;;) {
		int n = ibv_poll_cq(cq, 1, &wc);

		if (n < 0)
			die("ibv_poll_cq");
		if (n == 0) {
			usleep(1000);
			continue;
		}
		if (wc.status != IBV_WC_SUCCESS) {
			fprintf(stderr, "CQ completion failed: %s\n",
				ibv_wc_status_str(wc.status));
			exit(EXIT_FAILURE);
		}
		if (wc.wr_id == wr_id)
			return;
	}
}

static void exchange(void)
{
	struct ibv_sge sge;
	struct ibv_send_wr wr, *bad;
	int ret;

	snprintf(buf[0], MSG_SIZE, "Hello from RDMA client");
	sge.addr = (uintptr_t)buf[0];
	sge.length = strlen(buf[0]) + 1;
	sge.lkey = mr->lkey;

	memset(&wr, 0, sizeof(wr));
	wr.wr_id = WR_SEND;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.opcode = IBV_WR_SEND;
	wr.send_flags = IBV_SEND_SIGNALED;
	ret = ibv_post_send(qp, &wr, &bad);
	if (ret)
		die_verbs("ibv_post_send", ret);

	wait_cq(WR_SEND);
	wait_cq(WR_RECV);
	printf("server says: %s\n", buf[1]);
	fflush(stdout);
}

static void cleanup(void)
{
	if (qp)
		rdma_destroy_qp(id);
	if (mr)
		ibv_dereg_mr(mr);
	if (cq)
		ibv_destroy_cq(cq);
	if (pd)
		ibv_dealloc_pd(pd);
	if (id)
		rdma_destroy_id(id);
	if (channel)
		rdma_destroy_event_channel(channel);
}

int main(int argc, char **argv)
{
	const char *server = argc > 1 ? argv[1] : "127.0.0.1";
	const char *port = argc > 2 ? argv[2] : DEFAULT_PORT;
	struct addrinfo hints, *res;
	struct rdma_conn_param param;
	int ret;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	ret = getaddrinfo(server, port, &hints, &res);
	if (ret) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return EXIT_FAILURE;
	}

	channel = rdma_create_event_channel();
	if (!channel)
		die("rdma_create_event_channel");
	if (rdma_create_id(channel, &id, NULL, RDMA_PS_TCP))
		die("rdma_create_id");
	if (rdma_resolve_addr(id, NULL, res->ai_addr, 2000))
		die("rdma_resolve_addr");
	freeaddrinfo(res);
	wait_event(RDMA_CM_EVENT_ADDR_RESOLVED);

	if (rdma_resolve_route(id, 2000))
		die("rdma_resolve_route");
	wait_event(RDMA_CM_EVENT_ROUTE_RESOLVED);

	setup_qp();
	post_recv();

	memset(&param, 0, sizeof(param));
	param.responder_resources = 1;
	param.initiator_depth = 1;
	param.retry_count = 7;
	param.rnr_retry_count = 7;
	if (rdma_connect(id, &param))
		die("rdma_connect");
	wait_event(RDMA_CM_EVENT_ESTABLISHED);

	exchange();
	rdma_disconnect(id);
	wait_event(RDMA_CM_EVENT_DISCONNECTED);

	cleanup();
	return EXIT_SUCCESS;
}
