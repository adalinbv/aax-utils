

# Note Windows DLL are specified by RUNTIME

# Component support
#SET(CPACK_COMPONENTS_ALL Applications Libraries Headers)
SET(CPACK_COMPONENTS_ALL Applications)

# Display name
SET(CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "Applictaions")
SET(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C/C++ Development Files")

# Descriptions
SET(CPACK_COMPONENT_APPLICATIONS_DESCRIPTION
   "Support applictaions to test the capabilities of the software")
SET(CPACK_COMPONENT_LIBRARIES_DESCRIPTION
   "Dynamically shared components of the software")
SET(CPACK_COMPONENT_HEADERS_DESCRIPTION
   "Development header files and library components for use with the software")

# Dependecies
#SET(CPACK_COMPONENT_HEADERS_DEPENDS Libraries)
#SET(CPACK_COMPONENT_APPLICATIONS_DEPENDS Libraries)

# Component grouping
SET(CPACK_COMPONENT_APPLICATIONS_GROUP "Runtime")
SET(CPACK_COMPONENT_LIBRARIES_GROUP "Runtime")
SET(CPACK_COMPONENT_HEADERS_GROUP "Development")

SET(CPACK_COMPONENT_GROUP_RUNTIME_DESCRIPTION
   "Software required to run the software")
SET(CPACK_COMPONENT_GROUP_DEVELOPMENT_EXPANDED ON)
SET(CPACK_COMPONENT_GROUP_DEVELOPMENT_DESCRIPTION
   "C/C++ Development headers and libraries")

# NSIS only (for now?)
SET(CPACK_ALL_INSTALL_TYPES Runtime Developer)
SET(CPACK_COMPONENT_LIBRARIES_INSTALL_TYPES Runtime Developer)
SET(CPACK_COMPONENT_HEADERS_INSTALL_TYPES Developer)
SET(CPACK_COMPONENT_APPLICATIONS_INSTALL_TYPES Runtime)

