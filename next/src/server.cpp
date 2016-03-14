#include "multiverso/server.h"

#include "multiverso/actor.h"
#include "multiverso/table_interface.h" 
#include "multiverso/zoo.h"
#include "multiverso/util/io.h"

namespace multiverso {

Server::Server() : Actor(actor::kServer) {
  RegisterHandler(MsgType::Request_Get, std::bind(
    &Server::ProcessGet, this, std::placeholders::_1));
  RegisterHandler(MsgType::Request_Add, std::bind(
    &Server::ProcessAdd, this, std::placeholders::_1));
}

int Server::RegisterTable(ServerTable* server_table) {
  int id = static_cast<int>(store_.size());
  store_.push_back(server_table);
  return id;
}

void Server::ProcessGet(MessagePtr& msg) {
  MessagePtr reply(msg->CreateReplyMessage());
  int table_id = msg->table_id();
  CHECK(table_id >= 0 && table_id < store_.size());
  store_[table_id]->ProcessGet(msg->data(), &reply->data());
  SendTo(actor::kCommunicator, reply);
}

void Server::ProcessAdd(MessagePtr& msg) {
  MessagePtr reply(msg->CreateReplyMessage());
  int table_id = msg->table_id();
  CHECK(table_id >= 0 && table_id < store_.size());
  store_[table_id]->ProcessAdd(msg->data());
  SendTo(actor::kCommunicator, reply);
}

void Server::SetDumpFilePath(const std::string& dump_file_path){
  int id = Zoo::Get()->server_rank();
  std::string  server_id_str = (id == 0 ? "0" : "");
  while (id > 0){
    server_id_str = (char)((id % 10) + '0') + server_id_str;
    id /= 10;
  }
  dump_file_path_ = dump_file_path + server_id_str;
}

void Server::DumpTable(const int& epoch){
  std::shared_ptr<Stream> stream = StreamFactory::GetStream(URI(dump_file_path_), "w");
  stream->Write(&epoch, sizeof(int));
  char c = '\n';
  stream->Write(&c, sizeof(char));
  //std::ofstream os(dump_file_path_, std::ios::out);
  //os << epoch << c;
  for (int i = 0; i < store_.size(); ++i){
    store_[i]->DumpTable(stream);
    stream->Write(&c, sizeof(char));
    //os << c;
  }
  //os.close();
  stream->Flush();
}

int Server::RestoreTable(const std::string& file_path){
  std::shared_ptr<Stream> stream = StreamFactory::GetStream(URI(dump_file_path_), "r");
  //std::ifstream in(dump_file_path_, std::ios::in);
  int iter;
  char c;
  stream->Read(&iter, sizeof(int));
  stream->Read(&c, sizeof(char));
  //in >> iter;
  for (int i = 0; i < store_.size(); ++i){
    store_[i]->RecoverTable(stream);
    stream->Read(&c, sizeof(char));
  }
  stream->Flush();
  //in.close();
  return iter + 1; //the next iteration number
}
}