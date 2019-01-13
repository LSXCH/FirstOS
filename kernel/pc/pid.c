#include <zjunix/pid.h>

//初始化PID位图
void init_pid(){
    for(int i = 0; i < PID_BYTES; i++){
        pid_map[i] = 0;
    }
    pid_map[0] = 0x01;      //空进程pid
    next_pid = 1;
}

//检查PID是否已被使用
//已分配返回1，否则返回0
int pid_check(pid_t pid){
    //PID越界
    if(pid >= PID_NUM){
        return 0;
    }
    //查看位图中的该PID
    int map_index = pid >> 3;
    int byte_index = pid & 7;
    if(pid_map[map_index] & (1 << byte_index)){
        return 1;
    }
    else{
        return 0;
    }
}

//分配PID，ret_pid中存放分配的PID
//成功分配返回0，否则返回1
int pid_alloc(pid_t *ret_pid){
    int i;
    pid_t temp = next_pid;
    for(i = 0; i < PID_NUM; i++){
        if(pid_check(temp)){
            temp = (temp + 1) % PID_NUM;
        }
        else{
            break;
        }
    }
    if(i == PID_NUM){
        return 1;
    }
    int map_index = temp >>3;
    int byte_index = temp & 7;
    pid_map[map_index] |= (1 << byte_index);
    *ret_pid = temp;
    next_pid = (temp + 1) % PID_NUM;
    return 0;
}

//释放PID，成功返回0，否则返回1
int pid_free(pid_t pid){
    if(pid_check(pid)){
        int map_index = pid >> 3;
        int byte_index = pid & 7;
        pid_map[map_index] ^= (1 << byte_index);
        return 0;
    }
    else{
        return 1;
    }
}