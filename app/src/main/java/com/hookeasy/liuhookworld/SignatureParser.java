package com.hookeasy.liuhookworld;

import android.content.Context;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public class SignatureParser {

    static {
        System.loadLibrary("myhook"); // 改成你的解析库名（关键！）
    }

    private static final String TAG = "SignatureParser";

    public static native void nativeParseV1Signature(byte[] rsaBytes);

    public static void parseV1Signature(Context context, String apkPath) {
        ZipFile zipFile = null;
        try {
            zipFile = new ZipFile(apkPath);
            ZipEntry rsaEntry = null;

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

            byte[] rsaBytes = readEntryBytes(zipFile, rsaEntry);
            if (rsaBytes != null) {
                Log.i(TAG, "Read RSA length: " + rsaBytes.length);
                nativeParseV1Signature(rsaBytes); // 直接调用 JNI
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

    // 唯一修复：正确读取 ZipEntry 内容
// 兼容 API 26+：手动读取 InputStream 到 byte[]
    private static byte[] readEntryBytes(ZipFile zipFile, ZipEntry entry) {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        try (InputStream is = zipFile.getInputStream(entry)) {
            byte[] buffer = new byte[4096];
            int len;
            while ((len = is.read(buffer)) != -1) {
                baos.write(buffer, 0, len);
            }
            return baos.toByteArray();
        } catch (IOException e) {
            Log.e(TAG, "readEntryBytes error", e);
            return null;
        }
    }

    public static void parseSelfV1Signature(Context context) {
        try {
            parseV1Signature(context, context.getPackageCodePath());
        } catch (Exception e) {
            Log.e(TAG, "parseSelfV1Signature error", e);
        }
    }
}