//
// Created by Apple on 2020-02-04.
//

#include <jni.h>
#include <string>
#include "gif_lib.h"
#include <android/log.h>
#include <android/bitmap.h>

#define  LOG_TAG    "GifDecoder"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define  argb(a, r, g, b) ( ((a) & 0xff) << 24 ) | ( ((b) & 0xff) << 16 ) | ( ((g) & 0xff) << 8 ) | ((r) & 0xff)
typedef struct GifBean {
    // 当前播放的帧
    int current_frame;
    // 总的帧数
    int total_frame;
    // 每一帧的间隔时间 指针数组
    int *dealys;
};

/**
 * 绘制一帧图像
 * @param gifFileType
 * @param gifBean
 * @param info
 * @param pixels
 */
void drawFrame(GifFileType *gifFileType, GifBean *gifBean, AndroidBitmapInfo info, void *pixels) {
    // 获取当前帧的数据
    SavedImage savedImage = gifFileType->SavedImages[gifBean->current_frame];
    // 当前帧的图像信息
    GifImageDesc imageInfo = savedImage.ImageDesc;
    // 拿到图像数组的首地址
    int *px = (int *) pixels;
    // 图像颜色表
    ColorMapObject *colorMap = imageInfo.ColorMap;
    if (colorMap == nullptr) {
        colorMap = gifFileType->SColorMap;
    }
    // y 方向偏移量
    px = (int *) ((char *) px + info.stride * imageInfo.Top);
    // 像素点的位置
    int pointPixel;
    GifByteType gifByteType;//压缩数据
    //    每一行的首地址
    int *line;
    for (int y = imageInfo.Top; y < imageInfo.Top + imageInfo.Height; ++y) {
        line = px;
        for (int x = imageInfo.Left; x < imageInfo.Left + imageInfo.Width; ++x) {
            pointPixel = (y - imageInfo.Top) * imageInfo.Width + (x - imageInfo.Left);
            // 通过 LWZ 压缩算法拿到当前数组的值
            gifByteType = savedImage.RasterBits[pointPixel];
            GifColorType gifColorType = colorMap->Colors[gifByteType];
            // 将 color type 转换成 argb 的值
            line[x] = argb(255, gifColorType.Red, gifColorType.Green, gifColorType.Blue);
        }
        // 更新到下一行
        px = (int *) ((char *) px + info.stride);
    }
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_baidu_crazyorange_gifdecoder_gif_GifDecoder_loadGIF(JNIEnv *env, jclass type,
                                                             jstring path_) {
    const char *path = env->GetStringUTFChars(path_, 0);
    int errorCode = 0;
    // 根据路径获取 Gif 文件的结构
    GifFileType *gifFileType = DGifOpenFileName(path, &errorCode);
    // Gif 结构初始化，会填充上面读取的内容到 GifFileType 对象中
    DGifSlurp(gifFileType);
    // 给结构体 分配内存空间
    GifBean *gifBean = (GifBean *) malloc(sizeof(GifBean));
    memset(gifBean, 0, sizeof(GifBean));

    /**
     * 给 GifBean 赋值
     */
    // 给延时的总时间数组分配内存
    gifBean->dealys = static_cast<int *>(malloc(sizeof(int) * gifFileType->ImageCount));
    // 初始化了数组
    memset(gifBean->dealys, 0, sizeof(int) * gifFileType->ImageCount);

    // 图形拓展块
    ExtensionBlock *extensionBlock;

    for (int i = 0; i < gifFileType->ImageCount; ++i) {
        // 取出 GIF 中的每一帧
        SavedImage frame = gifFileType->SavedImages[i];
        // 拿到每一帧的拓展块
        for (int j = 0; j < frame.ExtensionBlockCount; ++j) {
            if (frame.ExtensionBlocks[j].Function == GRAPHICS_EXT_FUNC_CODE) {
                extensionBlock = &frame.ExtensionBlocks[j];
                break;
            }
            if (extensionBlock) {
                // 拿到图形控制拓展块和当前帧的延时时间
                // 因为它的 Bytes 是小端字节序 延时单位是 10 ms
                // Bytes[0] 是保留字段
                // Bytes[1] 是低 8 位
                // Bytes[2] 表示高 8 位
                int frame_delay = (extensionBlock->Bytes[2] << 8 | extensionBlock->Bytes[1]) * 10;
                gifBean->dealys[i] = frame_delay;
            }
        }
    }
    gifBean->total_frame = gifFileType->ImageCount;
    // 把这个数据交给 gifFileType 保存
    gifFileType->UserData = gifBean;

    env->ReleaseStringUTFChars(path_, path);
    return (jlong) gifFileType;
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_baidu_crazyorange_gifdecoder_gif_GifDecoder_getGifWidth(JNIEnv *env, jclass type,
                                                                 jlong gifPointer) {

    GifFileType *gifFileType = (GifFileType *) (gifPointer);

    return gifFileType->SWidth;
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_baidu_crazyorange_gifdecoder_gif_GifDecoder_getGifHeight(JNIEnv *env, jclass type,
                                                                  jlong gifPointer) {

    GifFileType *gifFileType = (GifFileType *) (gifPointer);

    return gifFileType->SHeight;
}


extern "C"
JNIEXPORT jint JNICALL
Java_com_baidu_crazyorange_gifdecoder_gif_GifDecoder_displayGif(JNIEnv *env, jclass type,
                                                                jobject bitmap, jlong gifPointer) {

    GifFileType *gifFileType = (GifFileType *) (gifPointer);
    GifBean *gifBean = (GifBean *) gifFileType->UserData;
    // 使用 AndroidBitmap 渲染 Bitmap
    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);
    // 锁定 Bitmap pixels 是创建一个像素数组，用来装 Gif 中的像素元素
    void *pixels;
    AndroidBitmap_lockPixels(env, bitmap, &pixels);
    drawFrame(gifFileType, gifBean, info, pixels);
    gifBean->current_frame += 1; // 绘制当前帧后加 1
    // 如果当前帧已经是最后一帧，代表它已经播完了，继续播放
    if (gifBean->current_frame >= gifBean->total_frame - 1) {
        gifBean->current_frame = 0;
        LOGE("重新过来  %d  ", gifBean->current_frame);
    }

    AndroidBitmap_unlockPixels(env, bitmap);
    return gifBean->dealys[gifBean->current_frame];
}