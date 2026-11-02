include(CPM)

# SYSTEM keeps toml++'s headers out of Miro's strict warning set; only
# Lib/Miro/TOML/Toml.cpp includes them.
CPMAddPackage(
        NAME tomlplusplus
        GITHUB_REPOSITORY marzer/tomlplusplus
        GIT_TAG v3.4.0
        SYSTEM YES)
