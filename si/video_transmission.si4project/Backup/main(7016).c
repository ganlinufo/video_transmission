#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <video_manager.h>
#include <convert_manager.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>

#define DelayTime 20   //(33us*1000=33ms 30f/s)

int iError;
int fd;
int i;

T_VideoDevice tVideoDevice;
T_VideoBuf tVideoBuf;
T_VideoBuf tConvertBuf;
PT_VideoConvert ptVideoConvert;
pthread_cond_t captureOK = PTHREAD_COND_INITIALIZER; /*线程采集满一个时的标志*/
pthread_cond_t encodeOK = PTHREAD_COND_INITIALIZER; /*线程编码完一个的标志*/
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;     /*互斥锁*/
pthread_t CaptureThread;
pthread_t EncodetTread;

static void init_fun(void)
{

	pthread_mutex_init(&lock,NULL);     /* 以动态方式创建互斥锁 */
	pthread_cond_init(&captureOK,NULL); /* 初始化captureOK条件变量 */
	pthread_cond_init(&encodeOK,NULL);  /* 初始化encodeOK条件变量 */
	 /* 申请帧内存,和转码帧内存 */
	memset(&tVideoBuf, 0, sizeof(tVideoBuf)); 
	memset(&tConvertBuf, 0, sizeof(tConvertBuf));

}

void YUY2toI420(int inWidth, int inHeight, unsigned char *pSrc, unsigned char *pDest)
{
    int i, j;
    //首先对I420的数据整体布局指定
    unsigned char *u = pDest + (inWidth * inHeight);
    unsigned char *v = u + (inWidth * inHeight) / 4;

    for (i = 0; i < inHeight/2; i++)
    {
        /*采取的策略是:在外层循环里面，取两个相邻的行*/    
        unsigned char *src_l1 = pSrc + inWidth*2*2*i; //因为4:2:2的原因，所以占用内存，相当一个像素占2个字节，2个像素合成4个字节
        unsigned char *src_l2 = src_l1 + inWidth*2;   //YUY2的偶数行下一行
        unsigned char *y_l1 = pDest + inWidth*2*i;    //偶数行
        unsigned char *y_l2 = y_l1 + inWidth;         //偶数行的下一行
        for (j = 0; j < inWidth/2; j++)         //内层循环
        {
            // two pels in one go//一次合成两个像素
            //偶数行，取完整像素;Y,U,V;偶数行的下一行，只取Y
            *y_l1++ = src_l1[0];   //Y
            *u++ = src_l1[1];      //U
            *y_l1++ = src_l1[2];   //Y
            *v++ = src_l1[3];      //V
            //这里只有取Y
            *y_l2++ = src_l2[0];
            *y_l2++ = src_l2[2];
            //YUY2,4个像素为一组
            src_l1 += 4;
            src_l2 += 4;
        }
    }
}


void *video_Capture_Thread(void *arg)
{
    /* 启动摄像头设备 */
    iError = tVideoDevice.ptOPr->StartDevice(&tVideoDevice);
	if(iError)
	{
		printf("StartDevice error!\n");
	}
	while(1)
	{
		long long absmsec;
        struct timeval now;  
        struct timespec outtime;  
    	gettimeofday(&now, NULL);
    	absmsec = now.tv_sec * 1000ll + now.tv_usec / 1000ll;
    	absmsec += DelayTime;
    	outtime.tv_sec = absmsec / 1000ll;
    	outtime.tv_nsec = absmsec % 1000ll * 1000000ll;
    	pthread_mutex_lock(&lock); /*获取互斥锁*/
    	
        /* 读入摄像头数据保存到帧内存 */
        /*注意pthread_mutex_lock时以下的资源将被锁定不能访问,所以pthread_mutex_lock单独不能放在
         *GetFrame的前面
         */
         
        pthread_cond_timedwait(&encodeOK,&lock,&outtime);
		if(tVideoDevice.ptOPr->GetFrame(&tVideoDevice, &tVideoBuf)==0)//采集一帧数据
		{
		    printf("GetFrame is ok\n");
		    pthread_cond_signal(&captureOK);
;
		}
		
		iError = tVideoDevice.ptOPr->PutFrame(&tVideoDevice, &tVideoBuf);
        if(iError)
        {
        	printf("PutFrame for error!\n");
        }
       
		pthread_mutex_unlock(&lock);     /*释放互斥锁*/
	}
}

void *video_Encode_Thread(void *arg)
{
    while (1)
	{			
	    pthread_mutex_lock(&lock); /* 获取互斥锁 */
        if(tVideoBuf.tPixelDatas.aucPixelDatas != NULL)
        {
            //printf("video_Encode_Thread in\n");
            //int h264_length = 0;
            pthread_cond_wait(&captureOK,&lock);
        	//h264_length = ptVideoConvert->Convert(&tVideoBuf, &tConvertBuf);/*H.264压缩视频*/
        	//if (h264_length <= 40000 && h264_length >0) 
        	//{
        	//	printf("h264_length = %d\n",h264_length);
        		//写h264文件
            //    if (write(fd,tConvertBuf.tPixelDatas.aucPixelDatas,h264_length)!=h264_length)
            //    {
            //       printf("write error\n");
             //   }
        	//}
        	YUY2toI420(WIDTH,HEIGHT,tVideoBuf.tPixelDatas.aucPixelDatas,tConvertBuf.tPixelDatas.aucPixelDatas);
		    write(fd,tConvertBuf.tPixelDatas.aucPixelDatas,I420);
    	}
    	else
    	{
    	    printf("Convert is NULL\n");
    	}
    	pthread_mutex_unlock(&lock);/*释放互斥锁*/	
	}
}


void thread_create(void)
{
        int temp;
        memset(&CaptureThread, 0, sizeof(CaptureThread));
        memset(&EncodetTread, 0, sizeof(EncodetTread));
		/*创建线程*/
        if((temp = pthread_create(&CaptureThread, NULL, video_Capture_Thread, NULL)) != 0)   
                printf("video_Capture_Thread create fail!\n");
        if((temp = pthread_create(&EncodetTread, NULL, video_Encode_Thread, NULL)) != 0)  
                printf("video_Encode_Thread create fail!\n");
}

void thread_wait(void)
{
    /*等待线程结束*/
    if(CaptureThread !=0) {  
            pthread_join(CaptureThread,NULL);
    }
    if(EncodetTread !=0) {   
            pthread_join(EncodetTread,NULL);
    }
}


int main(int argc, char *argv[])
{
    if (argc != 2)
    {
 		printf("Usage:\n");
		printf("%s </dev/video0,1,...>\n", argv[0]);
		return -1;
    }
    init_fun();
	video_init();
    iError = video_device_init(argv[1], &tVideoDevice);
    if (iError)
    {
        printf("VideoDeviceInit for %s error!\n", argv[1]);
        return -1;
    }
    video_convert_init();
    ptVideoConvert = get_video_convert_for_formats();
    if (iError)
    {
      printf("VideoDeviceInit for %s error!\n", argv[1]);
      return -1;
    }

   if ((fd = open("test.yuv",O_WRONLY|O_CREAT))<0)
    {
        printf("open error:\n");
        return -1;
    }
    thread_create();
    thread_wait();
    close(fd);
	return 0;
}




























