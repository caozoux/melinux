#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "greeter.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using greeter::Greeter;
using greeter::HelloRequest;
using greeter::HelloReply;

std::string ReadVmStatFile() {
    std::ifstream file("/proc/vmstat");
    if (!file.is_open()) {
        std::cerr << "Cannot open /proc/vmstat" << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char** argv) {
    // 1. 设置服务端地址，比如 localhost:50051
    std::string server_address("localhost:50051");

    // 2. 创建一个 gRPC channel，连接到服务端
    // 使用 insecure channel（无 TLS，仅用于测试和本地开发）
    std::shared_ptr<Channel> channel =
        grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());

    // 3. 检查 channel 是否可用
    if (!channel->GetState(true) == GRPC_CHANNEL_READY) {
        std::cerr << "Warning: 无法确认 Channel 是否已连接. 可能服务端未启动." << std::endl;
    }

    // 4. 创建一个 Stub（客户端存根），用于调用远程方法
    std::unique_ptr<Greeter::Stub> stub(Greeter::NewStub(channel));

    // 5. 准备请求：设置要发送的名字，比如 "World"
    HelloRequest request;

	// 1. 读取 /proc/vmstat
    std::string vmstat_content = ReadVmStatFile();
    if (vmstat_content.empty()) {
        return 1;
    }

    request.set_name(vmstat_content);

    // 6. 准备响应对象和上下文
    HelloReply reply;
    ClientContext context;

    // 7. 调用远程方法：SayHello
    std::cout << "Calling SayHello()... " << std::endl;

    Status status = stub->SayHello(&context, request, &reply);

    // 8. 检查调用是否成功
    if (status.ok()) {
        std::cout << "Greeter 服务端回复: " << reply.message() << std::endl;
    } else {
        std::cerr << "RPC 调用失败: " << status.error_code() << ": " << status.error_message() << std::endl;
    }

    return 0;
}
