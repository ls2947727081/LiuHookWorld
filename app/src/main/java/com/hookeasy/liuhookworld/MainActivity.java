package com.hookeasy.liuhookworld;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import com.hookeasy.liuhookworld.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'liuhookworld' library on application startup.
    static {
        System.loadLibrary("myhook");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());
    }

    /**
     * A native method that is implemented by the 'liuhookworld' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}