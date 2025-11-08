package com.hookeasy.liuhookworld;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public class SignatureParser {

    static {
        System.loadLibrary("myhook"); // 加载你的 so
    }

    private static final String TAG = "SignatureParser";

    // ----------- Native 方法声明 -----------
    public static native void nativeParseV1Signature(byte[] rsaBytes);
//    public static native void nativeParseV2Signature(byte[] v2Bytes);
//    public static native void nativeParseV3Signature(byte[] v3Bytes);
    // 可根据需要继续扩展

    /**
     * 从指定 APK 读取 META-INF 下的 RSA 文件并交给 native 层解析
     */
    public static void parseV1Signature(Context context, String apkPath) {
        ZipFile zipFile = null;
        try {
            zipFile = new ZipFile(apkPath);
            ZipEntry rsaEntry = null;

            // 查找 META-INF 下的 .RSA 文件
            for (ZipEntry entry : java.util.Collections.list(zipFile.entries())) {
                String name = entry.getName();
                if (name.startsWith("META-INF/") && name.endsWith(".RSA")) {
                    rsaEntry = entry;
                    break;
                }
            }

            if (rsaEntry == null) {
                Log.e(TAG, "No RSA file found in META-INF");
                return;
            }

            // 读取 RSA 文件为 byte[]
            byte[] rsaBytes = readEntryBytes(zipFile, rsaEntry);
            if (rsaBytes != null) {
                Log.i(TAG, "Read RSA length: " + rsaBytes.length);
                nativeParseV1Signature(rsaBytes);
            } else {
                Log.e(TAG, "Failed to read RSA entry");
            }

        } catch (Exception e) {
            Log.e(TAG, "parseV1Signature error", e);
        } finally {
            if (zipFile != null) {
                try {
                    zipFile.close();
                } catch (IOException ignore) {}
            }
        }
    }

    // 读取 ZipEntry 数据为 byte[]
    private static byte[] readEntryBytes(ZipFile zipFile, ZipEntry entry) throws IOException {
        try (FileInputStream in = new FileInputStream(new File(zipFile.getName()))) {
            return zipFile.getInputStream(entry).readAllBytes();
        }
    }

    /**
     * 从当前 App 自身 APK 读取 V1 签名
     */
    public static void parseSelfV1Signature(Context context) {
        try {
            PackageManager pm = context.getPackageManager();
            PackageInfo pi = pm.getPackageArchiveInfo(context.getPackageCodePath(), 0);
            if (pi != null) {
                parseV1Signature(context, context.getPackageCodePath());
            }
        } catch (Exception e) {
            Log.e(TAG, "parseSelfV1Signature error", e);
        }
    }
}
