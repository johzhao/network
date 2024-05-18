#ifndef ERROR_CODE_H
#define ERROR_CODE_H

enum ErrorCode {
    Success = 0,
    Not_Implement,
    Already_Initialized,

    // Socket Errors
    Socket_Error_Start = 0x00010000,
    Socket_Create_Failed = 0x00010101,
    Socket_Bind_Failed,
    Socket_Connect_Failed,
    Socket_Connect_In_Progress,
    Socket_Listen_Failed,
    Create_Epoll_Failed = 0x00010201,
    Add_Epoll_Event_Failed,
    Delete_Epoll_Event_Failed,
    Modify_Epoll_Event_Failed,

    // Utils Errors
    Utils_Error_Start = 0x000F0101,
    Buffer_Not_Enough_Capacity,
};

#endif //ERROR_CODE_H
