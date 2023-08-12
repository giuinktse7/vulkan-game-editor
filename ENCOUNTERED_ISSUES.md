# What is this?

This is a non-exhuastive list of issues that have been encountered while developing this project and their resolutions.

## QML calls destructor on instance created by C++

The problem is that QML assumes ownership of the instance and deletes it when it deems that it is no longer in use.

Fix:

-   use `QQmlEngine::setObjectOwnership(ptr, QQmlEngine::CppOwnership)`.
-   Set QObject parent of the instance:

```
TownData(uint32_t id, QString name, QObject *parent = nullptr);

std::make_unique<TownData>(value.id(), "Untitled", this)
```

## Project fails to compile because of QML lint issues

Remove the solutions suffixed with \_qmllint in Visual Studio and compilation will work.
