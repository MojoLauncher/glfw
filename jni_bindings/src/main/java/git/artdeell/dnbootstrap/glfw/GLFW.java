package git.artdeell.dnbootstrap.glfw;

import android.graphics.Bitmap;
import android.util.Log;
import android.view.Surface;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import git.artdeell.dnbootstrap.utils.Utils;

public class GLFW {
    public interface PosCallback {
        void receive(double x, double y);
    }
    public interface CursorCallback {
        void onCursorUse(GLFWCursor cursor);
    }
    private static GrabListener mGrabListener;
    private static Runnable onInitCallback;
    private static PosCallback mPositionCallback;
    private static CursorCallback mCursorCallback;
    private static GamepadEnableHandler gamepadEnabler;
    private static GLFWClipboard mClipboardImpl;
    public static ByteBuffer gamepadButtonBuffer;
    public static FloatBuffer gamepadAxisBuffer;

    public static final int GLFW_VISIBLE = 0x00020004;
    public static final int GLFW_HOVERED = 0x0002000B;

    static {
        System.loadLibrary("glfw");
        GLFW.initialize();
    }

    public static void setGamepadEnableHandler(GamepadEnableHandler handler) {
        GLFW.gamepadEnabler = handler;
    }

    public static void setGrabListener(GrabListener grabListener) {
        mGrabListener = grabListener;
    }

    public static void setClipboardImpl(GLFWClipboard mClipboardImpl) {
        GLFW.mClipboardImpl = mClipboardImpl;
    }
    public static void setPositionCallback(PosCallback callback){
        mPositionCallback = callback;
    }
    public static void setCursorCallback(CursorCallback callback){
        mCursorCallback = callback;
    }
    public static void setInitCallback(Runnable callback){
        onInitCallback = callback;
    }

    @SuppressWarnings("unused") // Used from native
    private static void receiveGrabState(boolean isGrabbing) {
        mGrabListener.onGrabState(isGrabbing);
    }

    @SuppressWarnings("unused") // Used from native
    private static void receiveCursorPos(double x, double y) {
        mPositionCallback.receive(x, y);
    }

    @SuppressWarnings("unused") // Used from native
    private static GLFWCursor loadCursor(ByteBuffer imageBytes, int width, int height, int xhot, int yhot) {
        try {
            Bitmap cursorBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            cursorBitmap.copyPixelsFromBuffer(imageBytes);
            return new GLFWCursor(cursorBitmap, xhot, yhot);
        }catch (Throwable t) {
            Log.w("GLFW", "Failed to load cursor", t);
            return null;
        }
    }

    @SuppressWarnings("unused") // Used from native
    private static void useCursor(GLFWCursor glfwCursor) {
        mCursorCallback.onCursorUse(glfwCursor);
    }

    @SuppressWarnings("unused") // Used from native
    private static String getClipboardString() {
        return mClipboardImpl.getClipboardString();
    }

    @SuppressWarnings("unused") // Used from native
    private static void setClipboardString(String str) {
        mClipboardImpl.setClipboardString(str);
    }

    @SuppressWarnings("unused") // Used from native
    private static void enableDirectGamepad(ByteBuffer buttonBuffer, ByteBuffer axisBuffer) {
        buttonBuffer = buttonBuffer.order(ByteOrder.nativeOrder());
        FloatBuffer axisFloatBuffer = axisBuffer.order(ByteOrder.nativeOrder()).asFloatBuffer();
        if(buttonBuffer.capacity() != 14 || axisFloatBuffer.capacity() != 6) {
            Log.i("GLFW", "Not enabling direct gamepad: unexpected buffer capacities ("+buttonBuffer.capacity()+" " + axisFloatBuffer.capacity()+")");
            return;
        }
        gamepadAxisBuffer = axisFloatBuffer;
        gamepadButtonBuffer = buttonBuffer;
        gamepadEnabler.onEnableGamepad();
    }

    @SuppressWarnings("unused") // Used from native
    public static void receiveInit() {
        onInitCallback.run();
    }

    public static native void initialize();
    public static native void sendMousePosition0(double x, double y);
    public static native void sendKeyEvent(int glfwCode, int state, int mods);
    public static native boolean sendRawKeyEvent(int androidCode, int state, int mods, char codepoint);
    public static native void sendMouseEvent(int glfwMouseKey, int state, int mods);
    public static native void sendBulkUnicodeEvent(String input, int mods);
    public static native void sendScrollEvent(double xoffset, double yoffset);
    public static native void nativeSurfaceCreated(Surface surface);
    public static native void nativeSurfaceDestroyed();
    public static native void nativeSurfaceUpdated();
    public static native void nativeNotifyGamepadConnected();
    public static native void nativeSetWindowAttribs(int attrib, boolean value);
}
