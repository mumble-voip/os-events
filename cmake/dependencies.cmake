include(FetchContent)

FetchContent_Declare(
	sdbus-c++
	GIT_REPOSITORY "https://github.com/Kistler-Group/sdbus-cpp.git"
	GIT_TAG        v2.2.1
	GIT_SHALLOW    ON
	FIND_PACKAGE_ARGS 2 # We need v2
)
