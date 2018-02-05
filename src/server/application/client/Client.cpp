#include <iostream>

#include "nn.h"
#include "reqrep.h"
#include "Login.pb.h"
#include "Strategy.pb.h"

#include "base/common/ProtoMessageCoder.h"
#include "model/Message.h"

using namespace std;

int sock = -1;
size_t n = 128;
char *buf = new char[n];

void Send(const std::shared_ptr<google::protobuf::Message> &msg)
{
  size_t size = base::EncodeProtoMessage(*msg, &buf, n);
  if (size > 0)
  {
    size_t bytes = nn_send(sock, buf, size, 0);
    if (unlikely(bytes != size))
    {
      cout << "Failed to send message(" << bytes << " != " << size << endl;
      return;
    }   
    char *recv_buf = NULL;
    size_t recv_bytes = nn_recv(sock, &recv_buf, NN_MSG, 0);
    if (recv_bytes > 0)
    {
      auto *m = base::DecodeProtoMessage(recv_buf, recv_bytes);
      if (m)
      {
        cout << m->GetTypeName() << " " << m->ShortDebugString() << endl;
      }
      else
      {
        cout << "Failed to decode message." << endl;
      }
    }
    else
    {
      cout << "Failed to receive reply message." << endl;
    }
  }
}

void Login()
{
  auto m = Message::NewProto<proto::Login>();
  m->set_user("pengchong");
  m->set_password("pengchong");
  m->set_version("0.0.2");
  m->set_ip("172.28.1.53");
  m->set_mac("50:65:f3:40:1d:f8");
  Send(m);
}

void Logout()
{
  auto m = Message::NewProto<proto::Logout>();
  m->set_user("pengchong");
  Send(m);
}

void Play()
{
  auto m = Message::NewProto<proto::StrategyStatusReq>();
  m->set_type(proto::Set);
  auto *s = m->add_statuses();
  s->set_name("test");
  s->set_underlying("m1809");
  s->set_status(proto::StrategyStatus::Play);
  m->set_user("pengchong");
  Send(m);
}

void Stop()
{
  auto m = Message::NewProto<proto::StrategyStatusReq>();
  m->set_type(proto::Set);
  auto *s = m->add_statuses();
  s->set_name("test");
  s->set_underlying("m1809");
  s->set_status(proto::StrategyStatus::Stop);
  m->set_user("pengchong");
  Send(m);
}

int main(int argc, char *argv[])
{
  cout << "Hello client." << endl;
  sock = nn_socket(AF_SP, NN_REQ);
  if (sock < 0)
  {
    cout << "Failed to create socket" << endl;
    return -1;
  }
  if (nn_connect(sock, "ipc:///tmp/reqrep.ipc") < 0)
  {
    cout << "Failed to connect socket" << endl;
    return -1;
  }

  char ch;
  bool running = true;
  while (running)
  {
    cout << "Press a key(1: Login|2: Logout|3: Play|4: Stop|q: exit)" << endl;
    cin >> ch;
    switch (ch)
    {
      case '1':
        Login();
        break;
      case '2':
        Logout();
        break;
      case '3':
        Play();
        break;
      case '4':
        Stop();
        break;
      case 'q':
        running = false;
        break;
      default:
        break;
    }
  }
  nn_close(sock);
}
