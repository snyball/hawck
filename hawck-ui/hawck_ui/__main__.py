from hawck_ui.main import main
import pkg_resources

version = "unknown-version"
try:
    version = pkg_resources.require("hawck_ui")[0].version
except:
    pass

main(version)
