package to.holepunch.bare.kit.react;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;

// Captures the application context at startup, as React Native never hands one
// to a C++ module, so that worklets can publish it to their native addons.
public class BareKitProvider extends ContentProvider {
  private static Context applicationContext;

  public static Context
  getApplicationContext() {
    return applicationContext;
  }

  @Override
  public boolean
  onCreate() {
    applicationContext = getContext().getApplicationContext();

    return false;
  }

  @Override
  public Cursor
  query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
    return null;
  }

  @Override
  public String
  getType(Uri uri) {
    return null;
  }

  @Override
  public Uri
  insert(Uri uri, ContentValues values) {
    return null;
  }

  @Override
  public int
  delete(Uri uri, String selection, String[] selectionArgs) {
    return 0;
  }

  @Override
  public int
  update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
    return 0;
  }
}
