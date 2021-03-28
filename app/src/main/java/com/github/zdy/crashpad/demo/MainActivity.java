package com.github.zdy.crashpad.demo;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("crashpad-lib");
    }

    private TextView tvInitResult;
    private Button btInitCrashPad;
    private Button btTestNativeCrash;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        initView();
    }

    private void initView() {
        tvInitResult = findViewById(R.id.tv_init_result);
        btInitCrashPad = findViewById(R.id.bt_init);
        btTestNativeCrash = findViewById(R.id.bt_native_crash);

        btInitCrashPad.setOnClickListener(view -> {
            boolean crashPadInitResult = initCrashPad(
                    getExternalFilesDir(null).getAbsolutePath(),
                    getApplicationInfo().nativeLibraryDir);
            tvInitResult.setText("初始化结果:" + crashPadInitResult);
            if (crashPadInitResult) {
                btInitCrashPad.setClickable(false);
            }
        });

        btTestNativeCrash.setOnClickListener(view -> {
            testNativeCrash();
        });
    }

    public native boolean initCrashPad(String path, String nativeLibDir);

    public native void testNativeCrash();

    public void test(){
        Log.d("CPD_MainActivity","test invoke.");
    }
}