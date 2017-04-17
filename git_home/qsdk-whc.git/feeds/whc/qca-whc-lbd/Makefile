include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qca-whc-lbd
PKG_SOURCE_PROTO:=git
PKG_BRANCH:=whc/banana
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
#  PKG_VERSION:=$(shell git ls-remote $(PKG_SOURCE_URL) $(PKG_BRANCH) | cut -b -7)
  PKG_VERSION:=3.0.1500.042
endif

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=$(PKG_VERSION)

WIFI_10_4_U:=PACKAGE_kmod-qca-wifi-10.4-unified-profile
WIFI_10_4_A:=PACKAGE_kmod-qca-wifi-10.4-akronite-perf
WIFI_10_4_D:=PACKAGE_kmod-qca-wifi-10.4-dakota-perf
DEF_WIFI_10_4_D:=DEFAULT_kmod-qca-wifi-10.4-dakota-perf
WIFI_10_4_MIPS:=PACKAGE_kmod-qca-wifi-10.4-perf
WIFI_10_2_MIPS:=PACKAGE_kmod-qca-wifi-perf
WIFI_10_2_ARM:=PACKAGE_kmod-qca-wifi-akronite-perf
WIFI_10_4_MIPS_UNIFIED:=PACKAGE_kmod-qca-wifi-10.4-unified-perf
WIFI_PKGS:=$(WIFI_10_4_U)||$(WIFI_10_4_A)||$(WIFI_10_4_D)||$(WIFI_10_4_MIPS)||$(WIFI_10_2_MIPS)||$(WIFI_10_2_ARM)||$(WIFI_10_4_MIPS_UNIFIED)||$(DEF_WIFI_10_4_D)

PKG_BUILD_DEPENDS:=$(DEF_WIFI_10_4_D):qca-wifi-10.4 $(WIFI_10_4_U):qca-wifi-10.4 $(WIFI_10_4_A):qca-wifi-10.4 $(WIFI_10_4_D):qca-wifi-10.4 \
	$(WIFI_10_2_MIPS):qca-wifi $(WIFI_10_2_ARM):qca-wifi $(WIFI_10_4_MIPS):qca-wifi-10.4 \
	$(WIFI_10_4_MIPS_UNIFIED):qca-wifi-10.4 libhyficommon $(patsubst PACKAGE_%,%,$(WIFI_10_4_U))


QCAWHCLBD_MAKE_OPTS:= \
        CROSS=$(TARGET_CROSS) \
        GWLIB=$(STAGING_DIR)/usr/lib \
        LBTOP=$(PKG_BUILD_DIR) \
        GWINCLUDE=$(STAGING_DIR)/usr/include/hyficommon \
        ATHDIR=$(STAGING_DIR)/usr/include \
        INSTALL_ROOT=$(PKG_BUILD_DIR)/install \
        STAGING_DIR=$(STAGING_DIR)

ifeq ($(CONFIG_LBD_SUPPORT_VHT160),y)
QCAWHCLBD_MAKE_OPTS+=LBD_SUPPORT_VHT160=y
endif

include $(INCLUDE_DIR)/package-version-override.mk
include $(INCLUDE_DIR)/package.mk

PLATFORM_PATCH_DIR:=./patches

define Package/qca-whc-lbd/config
  source "$(SOURCE)/Config.in"
  $(call Package/$(PKG_NAME)/override_version,$(PKG_NAME))
  $(call Package/$(PKG_NAME)/override_source_path,$(PKG_NAME))
endef

define Package/qca-whc-lbd
  SECTION:=QCA
  CATEGORY:=QCA Proprietary software
  URL:=http://www.qca.qualcomm.com
  MAINTAINER:=Qualcomm Atheros
  DEPENDS:=+qca-whc-init +libhyficommon +libpthread +@BUSYBOX_CONFIG_STAT \
	   +@BUSYBOX_CONFIG_FEATURE_STAT_FORMAT \
	   @$(WIFI_PKGS)
  TITLE:=Whole Home Coverage Load Balancing daemon
  SUBMENU:=Whole Home Coverage features
endef

define Package/qca-whc-lbd/description
  This package installs the Whole Home Coverage Load Balancing daemon
endef

ifneq ($(CONFIG_QCA_WHC_USE_DNI_GIT),)
PKG_GIT_DIR=qsdk-whc.git
PKG_GIT_FILES_SUBDIR=feeds/whc/qca-whc-lbd/files
PKG_GIT_SRC_SUBDIR=src/qca-whc-lbd

define Build/Prepare
	test x$(GIT_HOME) != x
	test -d $(GIT_HOME)/$(PKG_GIT_DIR)
	@echo "Plz git sync $(PKG_GIT_DIR) from dniserver/scm/dnigit/common/apps/$(PKG_GIT_DIR)"
	(cd $(GIT_HOME)/$(PKG_GIT_DIR); git cat-file -e $(CONFIG_WIFISON_WHC_GIT_TREEISH))
	mkdir -p $(PKG_BUILD_DIR)/files
	(cd $(PKG_BUILD_DIR)/files; git-archive --format=tar --remote=$(GIT_HOME)/$(PKG_GIT_DIR) $(CONFIG_WIFISON_WHC_GIT_TREEISH):$(PKG_GIT_FILES_SUBDIR) | tar -xvf -)
	(cd $(PKG_BUILD_DIR); git-archive --format=tar --remote=$(GIT_HOME)/$(PKG_GIT_DIR) $(CONFIG_WIFISON_WHC_GIT_TREEISH):$(PKG_GIT_SRC_SUBDIR) | tar -xvf -)
	$(if $(PLATFORM_PATCH_DIR), $(call PatchDir,$(PKG_BUILD_DIR),$(PLATFORM_PATCH_DIR),))
endef
endif

define Build/Compile
	mkdir -p $(PKG_BUILD_DIR)/install/lib
	mkdir -p $(PKG_BUILD_DIR)/install/include
	mkdir -p $(PKG_BUILD_DIR)/install/sbin
	$(MAKE) -C $(PKG_BUILD_DIR) $(strip $(QCAWHCLBD_MAKE_OPTS)) subdirs
endef

INSTALL_LIBS:= liblbcmnlibs
INSTALL_HEADERS=bandmon.h \
	bandmonCmn.h \
	diaglog.h \
	estimator.h \
	estimatorCmn.h \
	lb_assert.h \
	lb_common.h \
	lb_module.h \
	lb_profileSections.h \
	lbd_assert.h \
	lbd_types.h \
	stadbEntry.h \
	stadb.h  \
	stamon.h \
	stamonCmn.h \
	steeralg.h \
	steeralgCmn.h \
	steerexec.h \
	steerexecImplCmn.h \
	wlanif.h \
	wlanifBSteerControl.h \
	wlanifBSteerControlCmn.h \
	wlanifLinkEvents.h \
	wlanifLinkEventsCmn.h

define Package/qca-whc-lbd/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d
	$(if $(INSTALL_LIBS),$(CP) $(foreach lib,$(INSTALL_LIBS),$(PKG_BUILD_DIR)/install/lib/$(lib).so) $(1)/usr/lib)
	$(INSTALL_DIR) $(1)/lib/functions/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/install/sbin/lbd $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/lbt $(1)/usr/sbin/lbt
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/lbd.config $(1)/etc/config/lbd
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/lbd.init $(1)/etc/init.d/lbd
	$(CP) $(PKG_BUILD_DIR)/files/lbd-config.sh $(1)/lib/functions/
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/include/whc-wlb
	$(if $(INSTALL_LIBS),$(CP) $(foreach lib,$(INSTALL_LIBS),$(PKG_BUILD_DIR)/install/lib/$(lib).so) $(1)/usr/lib)
	$(if $(INSTALL_HEADERS),$(CP) $(foreach header,$(INSTALL_HEADERS),$(PKG_BUILD_DIR)/install/include/$(header)) $(1)/usr/include/whc-wlb)
endef

$(eval $(call BuildPackage,qca-whc-lbd))
