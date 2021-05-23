pragma Singleton
import QtQuick 2.15

QtObject {
  property FontLoader labelFontLoader : FontLoader {
    id : labelFontLoader
    source : "fonts/SourceSansPro-Regular.ttf"
  }
  readonly property alias labelFontFamily : labelFontLoader.name

  readonly property int defaultMargin : 16
  
  readonly property string labelTextColor: "#2b2b2b"
}
