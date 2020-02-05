package com.baidu.crazyorange.gifdecoder.gif;


import android.graphics.Bitmap;

/**
 * 使用 Native 库解码 GIF 文件
 */
public class GifDecoder {
    private long mGifPointer;

    static {
        System.loadLibrary("native-lib");
    }

    private GifDecoder(long gifPointer) {
        this.mGifPointer = gifPointer;
    }

    /**
     * 加载 GIF 文件,获取 Gif 的句柄
     *
     * @param path
     * @return
     */
    private static native long loadGIF(String path);

    private static native int getGifWidth(long gifPointer);

    private static native int getGifHeight(long gifPointer);

    private static native int displayGif(Bitmap bitmap, long gifPointer);

    private static GifDecoder load(String path) {
        long gifHandler = loadGIF(path);
        return new GifDecoder(gifHandler);
    }
}
