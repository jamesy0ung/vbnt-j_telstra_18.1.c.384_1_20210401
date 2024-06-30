#include "ril-daemon.c"

static void usage(char *s) {
    LOGE("usage: %s -d /dev/moderm_device -f file_path \n", s);
    LOGE("usage: or\n");
    LOGE("usage: %s -d /dev/moderm_device -u url_path \n", s);
}

static int ql_file_upload(const char *src_file, const char *dst_file) {
    ATResponse *p_response = NULL;
    int err;
    char *cmd;
    //char *line;
    int src_size;
    void *src_buf = NULL;
   // int dst_size = -1;
    FILE *fp = NULL;
    int ret = 0;

    fp = fopen(src_file, "rb");
    if (fp == NULL) {
        LOGE("Fail to open %s, errrno : %d (%s)\n", src_file, errno, strerror(errno));
        goto error;
    }

    fseek(fp, 0, SEEK_END);
    src_size = (int)ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    src_buf = (char *)malloc(src_size);
    if (src_buf == NULL) {
        LOGE("Fail to malloc file_size = %ld\n", src_size);
        goto error;
    }
        
    if (fread(src_buf, 1, src_size, fp) != src_size) {
        LOGE("Fail to fread file\n");
        goto error;
    } 

#if 0
    err = at_send_command_singleline("AT+QFLDS=\"UFS\"", "+QFLDS:", &p_response);
    
    if (err || p_response == NULL || p_response->success == 0)
        goto error;
    
    if (p_response != NULL) {at_response_free(p_response); p_response = NULL;};

    asprintf(&cmd, "AT+QFLST=\"%s\"", dst_file);
    err = at_send_command_singleline(cmd, "+QFLST:", &p_response);
    free(cmd);

    if (!err && p_response != NULL && p_response->success == 1) {
        //+QFLST: "Bluetooth_cal.acdb",1075
        char *filename;
        
        line = p_response->p_intermediates->line;
        
        err = at_tok_start(&line);
        if (err < 0) goto error;

        err = at_tok_nextstr(&line, &filename);
        if (err < 0) goto error;

        err = at_tok_nextint(&line, &dst_size);
        if (err < 0) goto error;
    }

    if (p_response != NULL) {at_response_free(p_response); p_response = NULL;};

    if (dst_size != src_size) {
        if (dst_size != -1) {
            asprintf(&cmd, "AT+QFDEL=\"%s\"", dst_file);
            err = at_send_command(cmd, &p_response);
            free(cmd);

            if (err || p_response == NULL || p_response->success == 0)
                goto error;


        if (p_response != NULL) {at_response_free(p_response); p_response = NULL;};
            sleep(1);
        }
#endif

        asprintf(&cmd, "AT+QFUPL=\"%s\",%d", dst_file, src_size);
        err = at_send_command_raw(cmd, src_buf, src_size, "+QFUPL:", &p_response);
        free(cmd);
		
        if (err || p_response == NULL || p_response->success == 0) {
            goto error;
        }
        
        if (p_response != NULL) {at_response_free(p_response); p_response = NULL;};

#if 0
	}
#endif

    ret = 1;

error:
    if (fp != NULL) {fclose(fp); fp = NULL;};
    if (src_buf != NULL) {free(src_buf); src_buf = NULL;};
    if (p_response != NULL) {at_response_free(p_response); p_response = NULL;};

    LOGD("%s(%s, %s) %s!\n", __func__, src_file, dst_file, ret ? "succ" : "fail");
    return ret;
}

int main (int argc, char **argv) {
    int opt;
    int modem_fd;
    char *device_path = "/dev/ttyUSB2";
    char *file_path = NULL;
    
    ATResponse *p_response = NULL;
    int err;
    LOGD("upgrade start...\n");
    while ( -1 != (opt = getopt(argc, argv, "f:u:d:"))) {
        switch (opt) {
            case 'd':
                device_path = optarg;
            break;
            case 'f':
                file_path = optarg;
            break;
            case 'u':
            break;            
            default:
                usage(argv[0]);
            break;
        }
    }

    if ((device_path == NULL) || ((file_path == NULL))) {
        usage(argv[0]);
        goto error;
    }

    modem_fd = serial_open(device_path);
    if (modem_fd < 0) {
           LOGE("Fail to open %s, errrno : %d (%s)\n", device_path, errno, strerror(errno));
           goto error;
    }
    
    at_set_on_reader_closed(onATReaderClosed);
    at_set_on_timeout(onATTimeout, 15000);

    at_send_command("ATE0Q0V1", NULL);
    err = at_send_command_multiline("ATI;+CSUB;+CVERSION", "\0", &p_response);
    if (err < 0 || p_response == NULL || p_response->success == 0) {
        goto error;
    } 
    if (!err && p_response && p_response->success) {
        ATLine *p_cur = p_response->p_intermediates;
        while (p_cur) {
            //LOGD("%s\n", p_cur->line);
            p_cur = p_cur->p_next;
        }
    }
    at_response_free(p_response);
	//LOGD("func = %s, line = %d\n", __FUNCTION__, __LINE__);

	if (0 != at_send_command("AT+QMBNCFG=\"Deactivate\"", NULL)) {
		 goto error;
	}
	//LOGD("func = %s, line = %d\n", __FUNCTION__, __LINE__);
	if (0 != at_send_command("AT+QMBNCFG=\"Delete\",\"Commercial-telstra\"", NULL)){
		 goto error;
	}
	//LOGD("func = %s, line = %d\n", __FUNCTION__, __LINE__);
	if(1 != ql_file_upload(file_path, "RAM:mcfg_sw.mbn")){
		 goto error;
	}
	//LOGD("func = %s, line = %d\n", __FUNCTION__, __LINE__);
	if (0 != at_send_command("at+qmbncfg=\"add\",\"RAM:mcfg_sw.mbn\"", NULL)){
		 goto error;
	}
	
	if (0 != at_send_command("at+qmbncfg=\"list\"", NULL)){
		 goto error;
	}
    	LOGD("upgrade success.\n");
		return 0;
	//waitForClose();
error:
    LOGD("upgrade fault.\n");

    return -1;
}
