

static struct rq_qos_ops blkcg_iolatency_ops = {
	.throttle = blkcg_iolatency_throttle,
	.done_bio = blkcg_iolatency_done_bio,
	.exit = blkcg_iolatency_exit,
}

return blkcg_policy_register(&blkcg_policy_iolatency);

return blkcg_policy_unregister(&blkcg_policy_iolatency);



