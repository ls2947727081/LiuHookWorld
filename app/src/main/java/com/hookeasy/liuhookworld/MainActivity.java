package com.hookeasy.liuhookworld;

import static com.hookeasy.liuhookworld.SignatureParser.parseV1Signature;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.View;
import android.widget.ScrollView;
import android.widget.TextView;
import android.util.Log;

import com.google.android.material.textfield.TextInputEditText;
import com.hookeasy.liuhookworld.databinding.ActivityMainBinding;

import java.io.BufferedReader;
import java.io.InputStreamReader;

import javax.security.auth.login.LoginException;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'liuhookworld' library on application startup.
    static {
        System.loadLibrary("myhook");
        System.loadLibrary("UnityDumper");
    }
    //本地方法
    public native String stringFromJNI();


    // 成员变量
    private TextInputEditText hint_text_2;
    private TextView Log_text_1;
    private ScrollView logScroll;
    private ActivityMainBinding binding;

    // 点击监听器对象
    private final View.OnClickListener buttonClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            // 点击按钮时获取输入文本
            String inputText = hint_text_2.getText().toString();

            Log.e("INFO", "hint_text_2 text: " + inputText);

            // 增行输出到日志框
            Log_text_1.append(inputText + "\n");

            // 如果 TextView 包裹在 ScrollView 中，可以自动滚动到底部
            logScroll.post(() -> logScroll.fullScroll(View.FOCUS_DOWN));
        }
    };


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.hnitText1;
        tv.setText(stringFromJNI());
        // 自动申请 root 权限
        RootUtils.requestRootPermission();

        //shadow加载


        hint_text_2 = binding.hnitText2;
        Log_text_1 = binding.logOutput;
        logScroll = binding.logScroll;
        // 给按钮绑定监听器
        binding.hintButtonOne.setOnClickListener(buttonClickListener);

        //v1签名解析
        // 自动解析当前 APK 的 V1 签名
        String apkPath = getApplicationInfo().sourceDir; // /data/app/.../base.apk
        parseV1Signature(this, apkPath);
    }

}