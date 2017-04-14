include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qca-wsplcd
PKG_RELEASE:=1

include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
#  PKG_REV:=$(shell git ls-remote $(PKG_SOURCE_URL) $(PKG_BRANCH) | cut -b -7)
#  PKG_VERSION:=g$(PKG_REV)
PKG_VERSION:=3.0.1500.042
endif
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

WIFI_10_4_U:=PACKAGE_kmod-qca-wifi-10.4-unified-profile
WIFI_10_4_A:=PACKAGE_kmod-qca-wifi-10.4-akronite-perf
WIFI_10_4_D:=PACKAGE_kmod-qca-wifi-10.4-dakota-perf
WIFI_10_4_MIPS:=PACKAGE_kmod-qca-wifi-10.4-perf
WIFI_10_4_MIPS_UNIFIED:=PACKAGE_kmod-qca-wifi-10.4-unified-perf
WIFI_10_2_MIPS:=PACKAGE_kmod-qca-wifi-perf
WIFI_10_2_ARM:=PACKAGE_kmod-qca-wifi-akronite-perf
WIFI_PKGS:=$(WIFI_10_4_U)||$(WIFI_10_4_A)||$(WIFI_10_4_D)||$(WIFI_10_4_MIPS)||$(WIFI_10_4_MIPS_UNIFIED)||$(WIFI_10_2_MIPS)||$(WIFI_10_2_ARM)
PKG_BUILD_DEPENDS:=$(WIFI_10_4_U):qca-wifi-10.4 $(WIFI_10_4_A):qca-wifi-10.4 $(WIFI_10_4_D):qca-wifi-10.4 $(WIFI_10_4_MIPS):qca-wifi-10.4 \
	$(WIFI_10_4_MIPS_UNIFIED):qca-wifi-10.4 $(WIFI_10_2_MIPS):qca-wifi $(WIFI_10_2_ARM):qca-wifi

QCAWSPLCD_MAKE_OPTS:= \
        CROSS=$(TARGET_CROSS) \
        GWLIB=$(STAGING_DIR)/usr/lib \
        GWINCLUDE=$(STAGING_DIR)/usr/include/hyficommon \
        WPA2INCLUDE=$(STAGING_DIR)/usr/include/wpa2 \
        HYDINCLUDE=$(STAGING_DIR)/usr/include/hyd \
        STORAGEINCLUE=$(STAGING_DIR)/usr/include/storage \
        IEEE1905INCLUDE=$(STAGING_DIR)/usr/include/ieee1905 \
        PLCSRV=$(STAGING_DIR)/usr/include/plcserv \
        ATHDIR=$(STAGING_DIR)/usr/include \
        INSTALL_ROOT=$(PKG_BUILD_DIR)/install \
        EXTRA_CFLAGS="$(TARGET_CFLAGS)" \
        EXTRA_LDFLAGS="$(TARGET_LDFLAGS)" \
        LIBSTORAGE_UCI=y

QCAWSPLCD_DEPENDS:=@$(WIFI_PKGS) +qca-ieee1905-init +libhyficommon +libstorage +libwpa2 +libieee1905 +libuci +libpthread
ifeq ($(CONFIG_WSPLCD_SUPPORT_PLC),y)
QCAWSPLCD_MAKE_OPTS+=WSPLCD_SUPPORT_PLC=y
QCAWSPLCD_DEPENDS+=+qca-plc-serv
endif

include $(INCLUDE_DIR)/package.mk

define Package/qca-wsplcd/config
  source "$(SOURCE)/Config.in"
  $(call Package/$(PKG_NAME)/override_version,$(PKG_NAME))
  $(call Package/$(PKG_NAME)/override_source_path,$(PKG_NAME))
endef

define Package/qca-wsplcd
  SECTION:=QCA
  CATEGORY:=QCA Proprietary software
  URL:=http://www.qca.qualcomm.com
  MAINTAINER:=Qualcomm Atheros, Inc.
  DEPENDS:=$(QCAWSPLCD_DEPENDS)
  TITLE:=IEEE 1905.1 AP Auto-Configuration daemon
  SUBMENU:=IEEE1905.1 features
endef

define Package/qca-wsplcd/description
  This package installs the IEEE 1905.1 AP Auto-Configuration daemon: Wi-Fi credential cloning over backbone network (e.g. HomePlugAV, Ethernet, Wi-Fi)
endef

ifneq ($(CONFIG_QCA_IEEE1905SECU_USE_DNI_GIT),)
PKG_GIT_DIR=qsdk-ieee1905-security.git
PKG_GIT_FILES_SUBDIR=feeds/ieee1905-security/qca-ieee1905-security-config/qca-wsplcd/files
PKG_GIT_SRC_SUBDIR=src/qca-wsplcd

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/files
	$(CP) $(GIT_HOME)/qsdk-ieee1905-security.git/$(PKG_GIT_FILES_SUBDIR)/*  $(PKG_BUILD_DIR)/files/
	$(CP) $(GIT_HOME)/qsdk-ieee1905-security.git/$(PKG_GIT_SRC_SUBDIR)/* $(PKG_BUILD_DIR)/
endef
endif

define Build/Compile
	mkdir -p $(PKG_BUILD_DIR)/install/etc
	mkdir -p $(PKG_BUILD_DIR)/install/sbin
	$(CP) $(PKG_BUILD_DIR)/wsplcd  $(PKG_BUILD_DIR)/install/sbin/
	$(CP) $(PKG_BUILD_DIR)/tools/*  $(PKG_BUILD_DIR)/install/sbin/
	$(CP) $(PKG_BUILD_DIR)/athx100.conf  $(PKG_BUILD_DIR)/install/etc/
endef

define Package/qca-wsplcd/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/install/sbin/wsplcd $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/install/sbin/ucpkgen $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/install/etc/athx100.conf $(1)/etc
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/wsplcd.init $(1)/etc/init.d/wsplcd
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/files/wsplcd.config $(1)/etc/config/wsplcd
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/uci2cfg.sh $(1)/sbin/uci2cfg.sh
	$(INSTALL_DIR) $(1)/etc/hotplug.d/button
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/wsplcd.button $(1)/etc/hotplug.d/button/40-wsplcd
endef

$(eval $(call BuildPackage,qca-wsplcd))
