#include <pistache/endpoint.h>

// test read    : curl http://127.0.0.1:9080/read_metadata/f1.txt.1
// test write   : curl --request POST  http://127.0.0.1:9080/write_metadata/f1.txt.1

using namespace Pistache;

struct CloudHandler : public Http::Handler {
    HTTP_PROTOTYPE(CloudHandler)

    void onRequest(const Http::Request& req, Http::ResponseWriter writer)
    {

        if (req.method() == Http::Method::Get)
        {
            std::string req_cmd = req.resource();
            std::string meta_cmd = "/read_metadata/";
            std::string block_cmd = "/read_block/";

            if (req_cmd.compare(0, meta_cmd.length(), meta_cmd) == 0)
            {
                std::string meta_name = req_cmd.substr(meta_cmd.length());
                // TODO : fetch metadata from redis and send back
                writer.send(Http::Code::Ok, meta_name);
            }
            else if (req_cmd.compare(0, block_cmd.length(), block_cmd) == 0)
            {
                std::string block_name = req_cmd.substr(block_cmd.length());

                // TODO : look-up block drive in Redis
                // TODO : fetch block and send back
                writer.send(Http::Code::Ok, block_name);
            }
            else
            {
                writer.send(Http::Code::Not_Found);
            }
        }
        else if (req.method() == Http::Method::Post)
        {
            std::string req_cmd = req.resource();
            std::string meta_cmd = "/write_metadata";
            std::string block_cmd = "/write_block";
            std::string content = req.body();
            if (req_cmd.compare(0, meta_cmd.length(), meta_cmd) == 0)
            {
                writer.send(Http::Code::Ok, "Write Metadata Succesfull");
            }
            else if (req_cmd.compare(0, block_cmd.length(), block_cmd) == 0)
            {
                writer.send(Http::Code::Ok, "Write Block Succesfull");
                // find (greedy) available harddrive based on file name

                // write the file content to the storage

                // write the metadata to REDIS
            }
            else
            {
                writer.send(Http::Code::Not_Found);
            }
        }
        else
        {
            writer.send(Http::Code::Not_Found);
        }
    }
};

int main() {
    // should receive args:
    // -ms metadata_server_ip -hd ip1;ip2;ip3;
    Http::listenAndServe<CloudHandler>("*:9080");
}
