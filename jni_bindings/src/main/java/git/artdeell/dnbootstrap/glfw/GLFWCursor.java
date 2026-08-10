package git.artdeell.dnbootstrap.glfw;

import android.graphics.Bitmap;

public class GLFWCursor {
    private final int xhot;
    private final int yhot;
    private final Bitmap bitmap;
    public GLFWCursor(Bitmap cursorBitmap, int xhot, int yhot) {
        this.xhot = xhot;
        this.yhot = yhot;
        this.bitmap = cursorBitmap;
    }

    public int getXhot() {
        return xhot;
    }

    public int getYhot() {
        return yhot;
    }

    public Bitmap getBitmap() {
        return bitmap;
    }
}
