pragma Singleton
import QtQuick 2.15

QtObject {
  property FontLoader labelFontLoader : FontLoader {
    id : labelFontLoader
    source : "SourceSansPro-Regular.ttf"
  }
  readonly property alias labelFontFamily : labelFontLoader.name

  readonly property int defaultMargin : 16
}
