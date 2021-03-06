#include <video_manager.h>
#include <string.h>

static PT_VideoOpr g_ptVideoOprHead = NULL;

/* 注册函数 */
int register_video_opr(PT_VideoOpr ptVideoOpr)
{
    PT_VideoOpr ptTmp;
    if (!g_ptVideoOprHead)
    {
        g_ptVideoOprHead = ptVideoOpr;
        ptVideoOpr->ptNext = NULL;
    }
    else
    {
        ptTmp = g_ptVideoOprHead;
        while (ptTmp->ptNext)
        {
            ptTmp = ptTmp->ptNext;
        }
        ptTmp->ptNext = ptVideoOpr;
        ptVideoOpr->ptNext = NULL;
    }
    return 0;
}

/* 显示函数 */
void show_video_opr(void)
{
    int i = 0;
    PT_VideoOpr ptTmp = g_ptVideoOprHead;
    while (ptTmp)
    {
        printf("%02d %s\n", i++, ptTmp->name);
        ptTmp = ptTmp->ptNext;
    }
}

/* 得到函数 */
PT_VideoOpr get_video_opr(char *pcName)
{
    PT_VideoOpr ptTmp = g_ptVideoOprHead;

    while (ptTmp)
    {
        if (strcpy(ptTmp->name, pcName) == 0)
        {
            return ptTmp;
        }
        ptTmp = ptTmp->ptNext;
    }
    return NULL
}

int video_device_init(char *strDevName, PT_VideoDevice ptVideoDevice)
{
    int iError;
    PT_VideoOpr ptTmp = g_ptVideoOprHead;
    while (ptTmp)
    {
        iError = ptTmp->InitDevice(strDevName, ptVideoDevice);
        if (!iError)
        {
            return 0;
        }
        ptTmp = ptTmp->ptNext;
    }
    return -1;
}

int VideoInit(void)
{
    int iError;

    iError = V4l2Init();

    return iError;
}
