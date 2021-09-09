#include "ConfigFileReader.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include "apnsclient.h"
#include "base_tool.h"

void init_global_library()
{
    SSL_library_init();
    SSL_load_error_strings();
}

void initApnsClient(uint32_t nsand_box, int nNums)
{
    CApnsClient* cli = NULL;
    if (nsand_box == 0) {
        cli = new CApnsClient(PUSH_CLIENT_TYPE_APNS);
    } else {
        cli = new CApnsClient(PUSH_CLIENT_TYPE_APNS_DEV);
    }
    cli->init(nNums);
}

int main(int argc, char* argv[])
{
#if 0
    pid_t pid = fork();
    if (pid < 0) {
        exit(-1);
    } else if (pid > 0) {
        exit(0);
    }
    setsid();
#endif
	if ((argc == 2) && (strcmp(argv[1], "-v") == 0)) {
		return 0;
	}

	signal(SIGPIPE, SIG_IGN);

    string file_name = "ipushserver.conf";
    CConfigFileReader config_file(file_name.c_str());

    char* topic = config_file.GetConfigName("topic");
    char* keyId = config_file.GetConfigName("keyId");
    char* teamId = config_file.GetConfigName("teamId");
    char* sand_box = config_file.GetConfigName("SandBox");
    char* authKey = config_file.GetConfigName("authKey");
    char* Nums = config_file.GetConfigName("connectNums");
    if (!topic || !keyId || !sand_box || !teamId || !authKey)
    {
        log("push app config file: %s not exist or miss required parameter obtained.", file_name.c_str());
        return 0;
    }
    int nNums = 0;
    if (Nums) 
    {
        nNums = atoi(Nums);
    }
    uint32_t nsand_box = atoi(sand_box);
    if (nsand_box != 1 && nsand_box != 0)
    {
        log("push app config parameter: sand_box has invaid value: %u.", nsand_box);
        return 0;
    }

    PUSH_CLIENT_TYPE pushType = PUSH_CLIENT_TYPE_APNS_DEV;
    if (nsand_box == 0) {
        pushType = PUSH_CLIENT_TYPE_APNS;
    }
    CHttp2Config* pHttpConf = CHttp2Config::GetInstance();
    pHttpConf->InitHttp2Config(std::string(topic), std::string(keyId), std::string(teamId), std::string(authKey), pushType);

    init_global_library();
    initApnsClient(nsand_box, nNums);

	return 0;
}
