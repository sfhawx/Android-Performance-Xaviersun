package com.zj.android.thread.hook

import android.os.Build
import android.os.Looper
import android.os.Process
import android.text.TextUtils
import android.util.Log
import androidx.annotation.RequiresApi

/**
 *  @Date:  2024/2/5
 *  @Author: xaviersun
 *  @Version: v1.0
 *  @Description:
 */
object ThreadMonitor {

    private val startThreads: MutableSet<Long> = mutableSetOf()
    private val releaseThreads:MutableSet<Long> = mutableSetOf()
    private val sLock = Any()
    private var number = 0

    @RequiresApi(Build.VERSION_CODES.M)
    @JvmStatic
    fun onPthreadCreated(threadID: Long): String {
        val stack = getMainThreadJavaStackTrace()
        // Log.d("thread_hook", "start java on thread create, id=${threadID}, stack=${stack}")
        if(stack.contains("onPthreadCreated")) {
            return stack
        }
        return ""
    }

    @RequiresApi(Build.VERSION_CODES.M)
    @JvmStatic
    fun onPthreadIdGet(threadID: Long) {
        synchronized(sLock) {
            startThreads.add(threadID)
        }
        number ++
        Log.d("thread_hook", "current number=${number}, startThreads size=${startThreads.size}, releaseThread size=${releaseThreads.size}")
    }

    @RequiresApi(Build.VERSION_CODES.M)
    @JvmStatic
    fun onPthreadReleased(threadID: Long) {
        synchronized(sLock) {
            releaseThreads.add(threadID)
        }
        Log.d("thread_hook", "end java on thread release, id=${threadID}")
    }

    private fun getMainThreadJavaStackTrace(): String {
        val stackTrace = StringBuilder()
        for (stackTraceElement in Looper.getMainLooper().thread.stackTrace) {
            stackTrace.append(stackTraceElement.toString()).append("\n")
        }
        return stackTrace.toString()
    }
}