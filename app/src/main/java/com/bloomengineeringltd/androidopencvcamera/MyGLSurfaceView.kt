package com.bloomengineeringltd.androidopencvcamera

import android.app.Activity
import android.content.Context
import android.opengl.GLSurfaceView
import android.os.Handler
import android.os.Looper
import android.util.AttributeSet
import android.util.Log
import android.view.MotionEvent
import android.view.View
import android.widget.TextView
import android.widget.Toast

class MyGLSurfaceView(context: Context, attrs: AttributeSet) : GLSurfaceView(context, attrs) {
/*    protected var frameCounter = 0
    protected var lastNanoTime: Long = 0
    var frontFacing = false
    var mFpsText: TextView? = null
    override fun onTouchEvent(e: MotionEvent): Boolean {
        if (e.action == MotionEvent.ACTION_DOWN) (context as Activity).openOptionsMenu()
        return true
    }

    var camRenderer: CamRenderer

    init {
        setEGLContextClientVersion(2)
        camRenderer = CamRenderer()
        setRenderer(camRenderer)
    }

    private fun initTimers() {
        frameCounter = 0
        lastNanoTime = System.nanoTime()
    }

    fun onCameraTexture(texIn: Int, texOut: Int, width: Int, height: Int): Boolean {
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
        external fun processFrame(texIn: Int, texOut: Int, w: Int, h: Int, frontFacing: Boolean)
    }*/
}