 package com.example.vulkan_android_depth;

import androidx.appcompat.app.AppCompatActivity;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.ImageDecoder;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import java.io.FileOutputStream;
import java.io.IOException;


 public class  MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }
     private AssetManager mgr;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = findViewById(R.id.sample_text);
        tv.setText(stringFromJNI());

        ImageView iv = findViewById(R.id.imageView);
        ImageView iv2 = findViewById(R.id.imageView2);
        ImageView iv3 = findViewById(R.id.imageView3);

        BitmapDrawable drawable0 = (BitmapDrawable) iv.getDrawable();
        Bitmap img0 = drawable0.getBitmap();
        BitmapDrawable drawable1 = (BitmapDrawable) iv2.getDrawable();
        Bitmap img1 = drawable1.getBitmap();
        mgr = getResources().getAssets();

        Button button = (Button) findViewById(R.id.button);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                String gpu_time = depth(mgr, img0, img1);
                Bitmap bitmap3 = img0.copy(img0.getConfig(), true);
                iv3.setImageBitmap(bitmap3);
                tv.setText("Took a total of : "+ gpu_time + "ms");
            }
        });
    }


    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native String depth(AssetManager assetManager, Bitmap img0, Bitmap img1);
}