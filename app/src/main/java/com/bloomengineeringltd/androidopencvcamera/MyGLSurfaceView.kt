package com.bloomengineeringltd.androidopencvcamera

import android.app.Activity
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.util.AttributeSet
import android.util.Log
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.View
import android.widget.TextView
import android.widget.Toast
import org.opencv.android.CameraGLSurfaceView
import org.opencv.android.CameraGLSurfaceView.CameraTextureListener

class MyGLSurfaceView(context: Context?, attrs: AttributeSet?) :
    CameraGLSurfaceView(context, attrs), CameraTextureListener {
    protected var frameCounter = 0
    protected var lastNanoTime: Long = 0
    protected var frontFacing = false
    var mFpsText: TextView? = null
    override fun onTouchEvent(e: MotionEvent): Boolean {
        if (e.action == MotionEvent.ACTION_DOWN) (context as Activity).openOptionsMenu()
        return true
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        super.surfaceCreated(holder)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        super.surfaceDestroyed(holder)
    }

    override fun onCameraViewStarted(width: Int, height: Int) {
        (context as Activity).runOnUiThread {
            Toast.makeText(
                context, "onCameraViewStarted", Toast.LENGTH_SHORT
            ).show()
        }
        frameCounter = 0
        lastNanoTime = System.nanoTime()
    }

    override fun onCameraViewStopped() {
        (context as Activity).runOnUiThread {
            Toast.makeText(
                context,
                "onCameraViewStopped",
                Toast.LENGTH_SHORT
            ).show()
        }
    }

    fun setFrontFacing(frontFacing: Boolean) {
        this.frontFacing = frontFacing
    }

    override fun onCameraTexture(texIn: Int, texOut: Int, width: Int, height: Int): Boolean {
        // FPS
        frameCounter++
        if (frameCounter >= 30) {
            val fps = (frameCounter * 1e9 / (System.nanoTime() - lastNanoTime)).toInt()
            Log.i(LOGTAG, "drawFrame() FPS: $fps")
            if (mFpsText != null) {
                val fpsUpdater = Runnable { mFpsText!!.text = "FPS: $fps" }
                Handler(Looper.getMainLooper()).post(fpsUpdater)
            } else {
                Log.d(LOGTAG, "mFpsText == null")
                mFpsText = (context as Activity).findViewById<View>(R.id.fps_text_view) as TextView
            }
            frameCounter = 0
            lastNanoTime = System.nanoTime()
        }
        processFrame(texIn, texOut, width, height, frontFacing)
        return true
    }

    companion object {
        const val LOGTAG = "MyGLSurfaceView"
        private external fun processFrame(
            texIn: Int,
            texOut: Int,
            w: Int,
            h: Int,
            frontFacing: Boolean
        )
    }
}