package com.zj.android.memory.hook

import com.bytedance.android.bytehook.ByteHook

/**
 *  @Date:  2024/2/4
 *  @Author: xaviersun
 *  @Version: v1.0
 *  @Description:
 */
class MemoryNativeLib {
    fun initHook() {
        ByteHook.init()
        hookMemory()
    }

    external fun hookMemory()

    external fun dump()

    companion object {
        // Used to load the 'hook' library on application startup.
        init {
            System.loadLibrary("memory-hook")
        }
    }
}