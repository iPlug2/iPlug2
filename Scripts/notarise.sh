#!/bin/sh -e

#============================================================
#   Before running this script, sign any apps with
#   hardened runtime:
#------------------------------------------------------------
#   codesign --force -s "$signID" -v "$appFile" --deep --strict --options=runtime
#   
#   echo "\nVerifying ..."
#   spctl -vvv --assess --type exec "$appFile"
#   codesign -dvv "$appFile"
#   codesign -vvv --deep --strict "$appFile"
#------------------------------------------------------------


#============================================================
# Notarise either a pkg installer, dmg or zip archive
#------------------------------------------------------------
#  Environment variables to set:
#  arg 1  Set to the absolute path where the script should run
#  arg 2  Set to the installer/dmg absolute path
#  arg 3  Set to the bundle ID
#  arg 4  Set to the app specific Apple username
#  arg 5  Set to the app specific Apple password
#------------------------------------------------------------

ROOT="$1"
APP_PATH="$2"
BUNDLE_ID="$3"
AC_USERNAME="$4"
AC_PASSWORD="$5"

cd $ROOT

if [ -z "$APP_PATH" ]; then
  echo "ERROR: First arg needs to be the path to the binary, dmg or pkg to notarise" && exit 1
fi

if [ -z "$BUNDLE_ID" ]; then
  echo "ERROR: Second arg needs to be a bundle ID to use e.g. com.company.product" && exit 1
fi

if [ -z "$AC_USERNAME" ]; then
  echo "ERROR: Third arg needs to be set the the app specific Apple username" && exit 1
fi

if [ -z "$AC_PASSWORD" ]; then
  echo "ERROR: Fourth arg needs to be set the the app specific Apple password" && exit 1
fi

#============================================================
# Setup variables
#============================================================
PATH_TO_NOTARISE="$APP_PATH"
EXTENSION="${PATH_TO_NOTARISE##*.}"
# echo "$EXTENSION"

TMP="$ROOT/tmp"
rm -rf "$TMP"
mkdir "$TMP"

function onExit {
  rm -rf "$TMP"
}
trap onExit EXIT

# Create zip to notarise
if [ "$EXTENSION" = "app" ]; then
  ZIP_PATH="$TMP/upload.zip"
  PATH_TO_NOTARISE="$ZIP_PATH"

  #============================================================
  # First check if notarization will succeed
  #============================================================
  echo "============================================================"
  echo "Validating file:"
  spctl -vvv --assess --type exec "$APP_PATH"
  codesign -dvv "$APP_PATH"
  codesign -vvv --deep --strict "$APP_PATH"

  # Create a ZIP archive suitable for altool
  rm -rf "$ZIP_PATH"
  /usr/bin/ditto -c -k --keepParent "$APP_PATH" "$ZIP_PATH"
fi

#============================================================
# Upload to notarization service
#============================================================
echo "============================================================"
echo "Uploading to notarization service:"
sudo xcode-select -s /Applications/Xcode.app

OUTPUT=$(xcrun altool --notarize-app --primary-bundle-id "$BUNDLE_ID" --username "$AC_USERNAME" --password "$AC_PASSWORD" --file "$PATH_TO_NOTARISE" --output-format xml)
OUTPUT_FILE="$TMP/result.plist"
rm -f "$OUTPUT_FILE"
echo "$OUTPUT" > "$OUTPUT_FILE"
REQUEST_UID=$(/usr/libexec/PlistBuddy -c "Print notarization-upload:RequestUUID" "$OUTPUT_FILE")
rm -f "$OUTPUT_FILE"
echo "$REQUEST_UID"


#============================================================
# Check status
#============================================================
echo "============================================================"
echo "Checking notarization status:"

for (( ; ; )); do
  STATUS_PLIST=$(xcrun altool --notarization-info "$REQUEST_UID" --username "$AC_USERNAME" --password "$AC_PASSWORD" --output-format xml)
  STATUS_FILE="$TMP/status.plist"
  rm -f "$STATUS_FILE"
  echo "$STATUS_PLIST" > "$STATUS_FILE"
  STATUS=$(/usr/libexec/PlistBuddy -c "Print notarization-info:Status" "$STATUS_FILE")
  echo "  STATUS: $STATUS"

  case "$STATUS" in
    "success")
      echo "  COMPLETED:" $(/usr/libexec/PlistBuddy -c "Print notarization-info:LogFileURL" "$STATUS_FILE")
      break
      ;;
    "in progress")
      sleep 5
      ;;
    *)
      echo "  ERROR:" $(/usr/libexec/PlistBuddy -c "Print notarization-info:'Status Message'" "$STATUS_FILE")
      echo "  LOG_URL:" $(/usr/libexec/PlistBuddy -c "Print notarization-info:LogFileURL" "$STATUS_FILE")
      exit 1
      ;;
  esac
done


#============================================================
# Staple to binary
#============================================================
echo "============================================================"
echo "Stapling certificate:"
xcrun stapler staple "$APP_PATH"
xcrun stapler staple -v "$APP_PATH"
