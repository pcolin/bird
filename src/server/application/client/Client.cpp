#include <iostream>
#include "nanomsg/nn.h"
#include "nanomsg/reqrep.h"
#include "Login.pb.h"
#include "Strategy.pb.h"
#include "base/common/ProtoMessageCoder.h"
#include "model/Message.h"

using namespace std;

int sock = -1;
size_t n = 128;
char *buf = new char[n];

void Send(const std::shared_ptr<google::protobuf::Message> &msg) {
  size_t size = base::EncodeProtoMessage(*msg, &buf, n);
  if (size > 0) {
    size_t bytes = nn_send(sock, buf, size, 0);
    if (unlikely(bytes != size)) {
      cout << "failed to send message(" << bytes << " != " << size << endl;
      return;
    }
    char *recv_buf = NULL;
    size_t recv_bytes = nn_recv(sock, &recv_buf, NN_MSG, 0);
    if (recv_bytes > 0) {
      auto *m = base::DecodeProtoMessage(recv_buf, recv_bytes);
      if (m) {
        cout << m->GetTypeName() << " " << m->ShortDebugString() << endl;
      } else {
        cout << "failed to decode message." << endl;
      }
    } else {
      cout << "failed to receive reply message." << endl;
    }
  }
}

void Login() {
  auto m = std::make_shared<Proto::Login>();
  m->set_user("pengchong");
  m->set_password("pengchong");
  m->set_role(Proto::Role::Trader);
  m->set_ip("172.28.1.53");
  m->set_mac("5065f3401df8");
  m->set_version("0.0.1");
  Send(m);
}

void Logout() {
  auto m = std::make_shared<Proto::Logout>();
  m->set_user("pengchong");
  Send(m);
}

void Play() {
  auto m = std::make_shared<Proto::StrategyOperateReq>();
  m->set_type(Proto::Set);
  auto *op = m->add_operates();
  op->set_name("test");
  op->set_underlying("m1809");
  op->set_operate(Proto::StrategyOperation::Start);
  m->set_user("pengchong");
  Send(m);
}

void Stop() {
  auto m = std::make_shared<Proto::StrategyOperateReq>();
  m->set_type(Proto::Set);
  auto *op = m->add_operates();
  op->set_name("test");
  op->set_underlying("m1809");
  op->set_operate(Proto::StrategyOperation::Stop);
  m->set_user("pengchong");
  Send(m);
}

int main(int argc, char *argv[]) {
  cout << "Hello client." << endl;
  sock = nn_socket(AF_SP, NN_REQ);
  if (sock < 0) {
    cout << "failed to create socket" << endl;
    return -1;
  }
  // if (nn_connect(sock, "ipc:///tmp/reqrep.ipc") < 0)
  if (nn_connect(sock, "tcp://172.28.1.53:8005") < 0) {
    cout << "failed to connect socket" << endl;
    return -1;
  }

  char ch;
  bool running = true;
  while (running) {
    cout << "press a key(1: Login|2: Logout|3: Play|4: Stop|q: exit)" << endl;
    cin >> ch;
    switch (ch) {
      case '1': {
        Login();
      break;
      }
      case '2': {
        Logout();
      break;
      }
      case '3': {
        Play();
      break;
      }
      case '4': {
        Stop();
      break;
      }
      case 'q': {
        running = false;
      break;
      }
      default: {
        break;
      }
    }
  }
  nn_close(sock);
}
