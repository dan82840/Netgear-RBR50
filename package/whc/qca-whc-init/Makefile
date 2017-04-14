include $(TOPDIR)/rules.mk

PKG_NAME:=qca-whc-init
PKG_RELEASE:=1
PKG_GIT_DIR=

PKG_VERSION:=3.0.1500.042
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/qca-whc-init
  SECTION:=QCA
  CATEGORY:=QCA Proprietary software
  URL:=http://www.qca.qualcomm.com
  MAINTAINER:=Qualcomm Atheros
  TITLE:=Whole Home Coverage Init Script Helpers
  DEPENDS:=
  SUBMENU:=Whole Home Coverage features
endef

define Package/qca-whc-init/config
  $(call Package/qca-whc-init/override_version,$(PKG_NAME))
  $(call Package/qca-whc-init/override_source_path,$(PKG_NAME))
  source "$(SOURCE)/Config.in"
endef

define Package/qca-whc-init/description
  This Package contains the script helper files for the Whole Home Coverage init scripts
endef

ifneq ($(CONFIG_QCA_WHC_USE_DNI_GIT),)
PKG_GIT_DIR=qsdk-whc.git
PKG_GIT_FILES_SUBDIR=feeds/whc/qca-whc-init/files

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/files
	$(CP) $(GIT_HOME)/qsdk-whc.git/$(PKG_GIT_FILES_SUBDIR)/* $(PKG_BUILD_DIR)/files/
endef
endif

define Package/qca-whc-init/install
	$(INSTALL_DIR) $(1)/lib/functions/
	$(CP) $(PKG_BUILD_DIR)/files/whc-debug.sh $(1)/lib/functions/
	$(CP) $(PKG_BUILD_DIR)/files/whc-iface.sh $(1)/lib/functions/
endef

define Build/Compile
	true
endef

$(eval $(call BuildPackage,qca-whc-init))
