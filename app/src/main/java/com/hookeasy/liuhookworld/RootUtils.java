package com.hookeasy.liuhookworld;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.io.*;
import java.util.concurrent.*;

public class RootUtils {
    private static final String TAG = "RootUtils";

    /** ---------------- 检查是否 root ---------------- */
    public static boolean isDeviceRooted() {
        String[] paths = {
                "/system/bin/su",
                "/system/xbin/su",
                "/sbin/su",
                "/data/local/xbin/su",
                "/data/local/bin/su",
                "/system/sd/xbin/su",
                "/su/bin/su",
                "/data/adb/magisk/su",
                "/system/bin/.ext/.su",
                "/system_ext/bin/su"   // <-- 新增
        };
        for (String path : paths) {
            if (new File(path).exists()) {
                Log.d(TAG, "su found at: " + path);
                return true;
            }
        }
        return false;
    }

    /** ---------------- Root 命令执行结果 ---------------- */
    public static class RootResult {
        public final boolean success;
        public final int exitCode;
        public final String stdout;
        public final String stderr;

        public RootResult(boolean success, int exitCode, String stdout, String stderr) {
            this.success = success;
            this.exitCode = exitCode;
            this.stdout = stdout;
            this.stderr = stderr;
        }
    }

    /** ---------------- 同步执行 Root 命令 ---------------- */
    public static RootResult executeRootCommandSync(String command, long timeoutSeconds) {
        if (!isDeviceRooted()) {
            Log.e(TAG, "Device is not rooted. Command cannot be executed.");
            return new RootResult(false, -1, "", "su not found");
        }

        Process process = null;
        DataOutputStream os = null;
        int exitCode = -1;
        final StringBuilder out = new StringBuilder();
        final StringBuilder err = new StringBuilder();

        ExecutorService gobblerPool = Executors.newFixedThreadPool(2);

        try {
            // 用 su 启动 shell
            process = Runtime.getRuntime().exec("su");

            // 分别读取 stdout/stderr 防止阻塞
            InputStream stdoutStream = process.getInputStream();
            InputStream stderrStream = process.getErrorStream();
            Future<?> outFuture = gobblerPool.submit(() -> streamToBuilder(stdoutStream, out));
            Future<?> errFuture = gobblerPool.submit(() -> streamToBuilder(stderrStream, err));

            os = new DataOutputStream(process.getOutputStream());
            os.writeBytes(command + "\n");
            os.writeBytes("exit\n");
            os.flush();

            boolean finished = process.waitFor(timeoutSeconds, TimeUnit.SECONDS);
            if (!finished) {
                process.destroyForcibly();
                Log.e(TAG, "Root command timeout, destroyed process.");
                exitCode = -1;
            } else {
                exitCode = process.exitValue();
            }

            try { outFuture.get(1, TimeUnit.SECONDS); } catch (Exception ignored) {}
            try { errFuture.get(1, TimeUnit.SECONDS); } catch (Exception ignored) {}

            boolean success = (exitCode == 0);
            Log.d(TAG, "Root command exitCode=" + exitCode + " stdout=" + out + " stderr=" + err);
            return new RootResult(success, exitCode, out.toString(), err.toString());

        } catch (IOException e) {
            Log.e(TAG, "IOException while running root command.", e);
            return new RootResult(false, -2, out.toString(), e.getMessage());
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return new RootResult(false, -3, out.toString(), "Interrupted");
        } finally {
            gobblerPool.shutdownNow();
            try { if (os != null) os.close(); } catch (Exception ignored) {}
            if (process != null) process.destroy();
        }
    }

    /** ---------------- 异步执行 Root 命令 ---------------- */
    public interface RootCallback {
        void onResult(RootResult result);
    }

    public static void executeRootCommandAsync(String command, long timeoutSeconds, RootCallback callback) {
        ExecutorService pool = Executors.newSingleThreadExecutor();
        pool.submit(() -> {
            RootResult r = executeRootCommandSync(command, timeoutSeconds);
            if (callback != null) callback.onResult(r);
        });
        pool.shutdown();
    }

    /** ---------------- 流读取工具 ---------------- */
    private static void streamToBuilder(InputStream stream, StringBuilder builder) {
        try (BufferedReader br = new BufferedReader(new InputStreamReader(stream))) {
            String line;
            while ((line = br.readLine()) != null) builder.append(line).append('\n');
        } catch (IOException e) { /* ignore */ }
    }

    /** ---------------- 常见 Root 场景封装 ---------------- */

    // 创建文件夹
    public static RootResult mkdir(String path) {
        return executeRootCommandSync("mkdir -p " + path, 10);
    }

    // 删除文件/文件夹
    public static RootResult rm(String path) {
        return executeRootCommandSync("rm -rf " + path, 10);
    }

    // 修改权限
    public static RootResult chmod(String path, String mode) {
        return executeRootCommandSync("chmod " + mode + " " + path, 10);
    }

    // 修改所有者
    public static RootResult chown(String path, String owner) {
        return executeRootCommandSync("chown " + owner + " " + path, 10);
    }

    // 写入文本文件（覆盖）
    public static RootResult writeFile(String path, String content) {
        String cmd = "echo \"" + content.replace("\"", "\\\"") + "\" > " + path;
        return executeRootCommandSync(cmd, 10);
    }

    // 读取文件内容
    public static RootResult readFile(String path) {
        return executeRootCommandSync("cat " + path, 10);
    }

    /** ---------------- 加载本地 so 并用 root 权限操作 ---------------- */
    public static RootResult loadNativeSo(String soPath) {
        File soFile = new File(soPath);
        if (!soFile.exists()) {
            return new RootResult(false, -1, "", "so file not found: " + soPath);
        }

        // 用 root 权限设置可执行并加载
        RootResult chmodRes = chmod(soPath, "755");
        if (!chmodRes.success) return chmodRes;

        // 加载 so
        try {
            System.load(soPath); // 或者用 dlopen 方式
            return new RootResult(true, 0, "Loaded " + soPath, "");
        } catch (UnsatisfiedLinkError e) {
            return new RootResult(false, -2, "", e.getMessage());
        }
    }

    /**
     * 启动 app 时请求 root 授权
     * 会在 UI 线程触发 su，确保 Magisk 弹窗出现
     */
    public static void requestRootPermission() {
        if (!isDeviceRooted()) {
            Log.e(TAG, "Device not rooted, cannot request root.");
            return;
        }

        // UI 线程触发 Magisk 弹窗
        new Handler(Looper.getMainLooper()).post(() -> {
            executeRootCommandAsync("id", 10, result -> {
                if (result.success) {
                    Log.d(TAG, "Root permission granted: " + result.stdout);
                } else {
                    Log.e(TAG, "Root permission request failed: " + result.stderr);
                }
            });
        });
    }

}
