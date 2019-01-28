// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/07.

#include "polarphp/basic/adt/Triple.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

TEST(TripleTest, testBasicParsing)
{
   Triple T;

   T = Triple("");
   EXPECT_EQ("", T.getArchName().getStr());
   EXPECT_EQ("", T.getVendorName().getStr());
   EXPECT_EQ("", T.getOSName().getStr());
   EXPECT_EQ("", T.getEnvironmentName().getStr());

   T = Triple("-");
   EXPECT_EQ("", T.getArchName().getStr());
   EXPECT_EQ("", T.getVendorName().getStr());
   EXPECT_EQ("", T.getOSName().getStr());
   EXPECT_EQ("", T.getEnvironmentName().getStr());

   T = Triple("--");
   EXPECT_EQ("", T.getArchName().getStr());
   EXPECT_EQ("", T.getVendorName().getStr());
   EXPECT_EQ("", T.getOSName().getStr());
   EXPECT_EQ("", T.getEnvironmentName().getStr());

   T = Triple("---");
   EXPECT_EQ("", T.getArchName().getStr());
   EXPECT_EQ("", T.getVendorName().getStr());
   EXPECT_EQ("", T.getOSName().getStr());
   EXPECT_EQ("", T.getEnvironmentName().getStr());

   T = Triple("----");
   EXPECT_EQ("", T.getArchName().getStr());
   EXPECT_EQ("", T.getVendorName().getStr());
   EXPECT_EQ("", T.getOSName().getStr());
   EXPECT_EQ("-", T.getEnvironmentName().getStr());

   T = Triple("a");
   EXPECT_EQ("a", T.getArchName().getStr());
   EXPECT_EQ("", T.getVendorName().getStr());
   EXPECT_EQ("", T.getOSName().getStr());
   EXPECT_EQ("", T.getEnvironmentName().getStr());

   T = Triple("a-b");
   EXPECT_EQ("a", T.getArchName().getStr());
   EXPECT_EQ("b", T.getVendorName().getStr());
   EXPECT_EQ("", T.getOSName().getStr());
   EXPECT_EQ("", T.getEnvironmentName().getStr());

   T = Triple("a-b-c");
   EXPECT_EQ("a", T.getArchName().getStr());
   EXPECT_EQ("b", T.getVendorName().getStr());
   EXPECT_EQ("c", T.getOSName().getStr());
   EXPECT_EQ("", T.getEnvironmentName().getStr());

   T = Triple("a-b-c-d");
   EXPECT_EQ("a", T.getArchName().getStr());
   EXPECT_EQ("b", T.getVendorName().getStr());
   EXPECT_EQ("c", T.getOSName().getStr());
   EXPECT_EQ("d", T.getEnvironmentName().getStr());
}

TEST(TripleTest, testParsedIDs)
{
   Triple T;

   T = Triple("i386-apple-darwin");
   EXPECT_EQ(Triple::ArchType::x86, T.getArch());
   EXPECT_EQ(Triple::VendorType::Apple, T.getVendor());
   EXPECT_EQ(Triple::OSType::Darwin, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("i386-pc-elfiamcu");
   EXPECT_EQ(Triple::ArchType::x86, T.getArch());
   EXPECT_EQ(Triple::VendorType::PC, T.getVendor());
   EXPECT_EQ(Triple::OSType::ELFIAMCU, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("i386-pc-contiki-unknown");
   EXPECT_EQ(Triple::ArchType::x86, T.getArch());
   EXPECT_EQ(Triple::VendorType::PC, T.getVendor());
   EXPECT_EQ(Triple::OSType::Contiki, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("i386-pc-hurd-gnu");
   EXPECT_EQ(Triple::ArchType::x86, T.getArch());
   EXPECT_EQ(Triple::VendorType::PC, T.getVendor());
   EXPECT_EQ(Triple::OSType::Hurd, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());

   T = Triple("x86_64-pc-linux-gnu");
   EXPECT_EQ(Triple::ArchType::x86_64, T.getArch());
   EXPECT_EQ(Triple::VendorType::PC, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());

   T = Triple("x86_64-pc-linux-musl");
   EXPECT_EQ(Triple::ArchType::x86_64, T.getArch());
   EXPECT_EQ(Triple::VendorType::PC, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::Musl, T.getEnvironment());

   T = Triple("powerpc-bgp-linux");
   EXPECT_EQ(Triple::ArchType::ppc, T.getArch());
   EXPECT_EQ(Triple::VendorType::BGP, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("powerpc-bgp-cnk");
   EXPECT_EQ(Triple::ArchType::ppc, T.getArch());
   EXPECT_EQ(Triple::VendorType::BGP, T.getVendor());
   EXPECT_EQ(Triple::OSType::CNK, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("ppc-bgp-linux");
   EXPECT_EQ(Triple::ArchType::ppc, T.getArch());
   EXPECT_EQ(Triple::VendorType::BGP, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("ppc32-bgp-linux");
   EXPECT_EQ(Triple::ArchType::ppc, T.getArch());
   EXPECT_EQ(Triple::VendorType::BGP, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("powerpc64-bgq-linux");
   EXPECT_EQ(Triple::ArchType::ppc64, T.getArch());
   EXPECT_EQ(Triple::VendorType::BGQ, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("ppc64-bgq-linux");
   EXPECT_EQ(Triple::ArchType::ppc64, T.getArch());
   EXPECT_EQ(Triple::VendorType::BGQ, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());

   T = Triple("powerpc-ibm-aix");
   EXPECT_EQ(Triple::ArchType::ppc, T.getArch());
   EXPECT_EQ(Triple::VendorType::IBM, T.getVendor());
   EXPECT_EQ(Triple::OSType::AIX, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("powerpc64-ibm-aix");
   EXPECT_EQ(Triple::ArchType::ppc64, T.getArch());
   EXPECT_EQ(Triple::VendorType::IBM, T.getVendor());
   EXPECT_EQ(Triple::OSType::AIX, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("powerpc-dunno-notsure");
   EXPECT_EQ(Triple::ArchType::ppc, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("arm-none-none-eabi");
   EXPECT_EQ(Triple::ArchType::arm, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::EABI, T.getEnvironment());

   T = Triple("arm-none-linux-musleabi");
   EXPECT_EQ(Triple::ArchType::arm, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::MuslEABI, T.getEnvironment());

   T = Triple("armv6hl-none-linux-gnueabi");
   EXPECT_EQ(Triple::ArchType::arm, T.getArch());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNUEABI, T.getEnvironment());

   T = Triple("armv7hl-none-linux-gnueabi");
   EXPECT_EQ(Triple::ArchType::arm, T.getArch());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNUEABI, T.getEnvironment());

   T = Triple("amdil-unknown-unknown");
   EXPECT_EQ(Triple::ArchType::amdil, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());

   T = Triple("amdil64-unknown-unknown");
   EXPECT_EQ(Triple::ArchType::amdil64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());

   T = Triple("hsail-unknown-unknown");
   EXPECT_EQ(Triple::ArchType::hsail, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());

   T = Triple("hsail64-unknown-unknown");
   EXPECT_EQ(Triple::ArchType::hsail64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());

   T = Triple("sparcel-unknown-unknown");
   EXPECT_EQ(Triple::ArchType::sparcel, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());

   T = Triple("spir-unknown-unknown");
   EXPECT_EQ(Triple::ArchType::spir, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());

   T = Triple("spir64-unknown-unknown");
   EXPECT_EQ(Triple::ArchType::spir64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());

   T = Triple("x86_64-unknown-ananas");
   EXPECT_EQ(Triple::ArchType::x86_64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Ananas, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("x86_64-unknown-cloudabi");
   EXPECT_EQ(Triple::ArchType::x86_64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::CloudABI, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("x86_64-unknown-fuchsia");
   EXPECT_EQ(Triple::ArchType::x86_64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Fuchsia, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("x86_64-unknown-hermit");
   EXPECT_EQ(Triple::ArchType::x86_64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::HermitCore, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("wasm32-unknown-unknown");
   EXPECT_EQ(Triple::ArchType::wasm32, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("wasm64-unknown-unknown");
   EXPECT_EQ(Triple::ArchType::wasm64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("avr-unknown-unknown");
   EXPECT_EQ(Triple::ArchType::avr, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("avr");
   EXPECT_EQ(Triple::ArchType::avr, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("lanai-unknown-unknown");
   EXPECT_EQ(Triple::ArchType::lanai, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("lanai");
   EXPECT_EQ(Triple::ArchType::lanai, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("amdgcn-mesa-mesa3d");
   EXPECT_EQ(Triple::ArchType::amdgcn, T.getArch());
   EXPECT_EQ(Triple::VendorType::Mesa, T.getVendor());
   EXPECT_EQ(Triple::OSType::Mesa3D, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("amdgcn-amd-amdhsa");
   EXPECT_EQ(Triple::ArchType::amdgcn, T.getArch());
   EXPECT_EQ(Triple::VendorType::AMD, T.getVendor());
   EXPECT_EQ(Triple::OSType::AMDHSA, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("amdgcn-amd-amdpal");
   EXPECT_EQ(Triple::ArchType::amdgcn, T.getArch());
   EXPECT_EQ(Triple::VendorType::AMD, T.getVendor());
   EXPECT_EQ(Triple::OSType::AMDPAL, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("riscv32-unknown-unknown");
   EXPECT_EQ(Triple::ArchType::riscv32, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("riscv64-unknown-linux");
   EXPECT_EQ(Triple::ArchType::riscv64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("riscv64-unknown-freebsd");
   EXPECT_EQ(Triple::ArchType::riscv64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::FreeBSD, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("armv7hl-suse-linux-gnueabi");
   EXPECT_EQ(Triple::ArchType::arm, T.getArch());
   EXPECT_EQ(Triple::VendorType::SUSE, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNUEABI, T.getEnvironment());

   T = Triple("i586-pc-haiku");
   EXPECT_EQ(Triple::ArchType::x86, T.getArch());
   EXPECT_EQ(Triple::VendorType::PC, T.getVendor());
   EXPECT_EQ(Triple::OSType::Haiku, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("x86_64-unknown-haiku");
   EXPECT_EQ(Triple::ArchType::x86_64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Haiku, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("mips-mti-linux-gnu");
   EXPECT_EQ(Triple::ArchType::mips, T.getArch());
   EXPECT_EQ(Triple::VendorType::MipsTechnologies, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());

   T = Triple("mipsel-img-linux-gnu");
   EXPECT_EQ(Triple::ArchType::mipsel, T.getArch());
   EXPECT_EQ(Triple::VendorType::ImaginationTechnologies, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());

   T = Triple("mips64-mti-linux-gnu");
   EXPECT_EQ(Triple::ArchType::mips64, T.getArch());
   EXPECT_EQ(Triple::VendorType::MipsTechnologies, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());

   T = Triple("mips64el-img-linux-gnu");
   EXPECT_EQ(Triple::ArchType::mips64el, T.getArch());
   EXPECT_EQ(Triple::VendorType::ImaginationTechnologies, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());

   T = Triple("mips64el-img-linux-gnuabin32");
   EXPECT_EQ(Triple::ArchType::mips64el, T.getArch());
   EXPECT_EQ(Triple::VendorType::ImaginationTechnologies, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNUABIN32, T.getEnvironment());

   T = Triple("mips64el-unknown-linux-gnuabi64");
   EXPECT_EQ(Triple::ArchType::mips64el, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNUABI64, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::NoSubArch, T.getSubArch());
   T = Triple("mips64el");
   EXPECT_EQ(Triple::ArchType::mips64el, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNUABI64, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::NoSubArch, T.getSubArch());

   T = Triple("mips64-unknown-linux-gnuabi64");
   EXPECT_EQ(Triple::ArchType::mips64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNUABI64, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::NoSubArch, T.getSubArch());
   T = Triple("mips64");
   EXPECT_EQ(Triple::ArchType::mips64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNUABI64, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::NoSubArch, T.getSubArch());

   T = Triple("mipsisa64r6el-unknown-linux-gnuabi64");
   EXPECT_EQ(Triple::ArchType::mips64el, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNUABI64, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());
   T = Triple("mips64r6el");
   EXPECT_EQ(Triple::ArchType::mips64el, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNUABI64, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());
   T = Triple("mipsisa64r6el");
   EXPECT_EQ(Triple::ArchType::mips64el, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNUABI64, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());

   T = Triple("mipsisa64r6-unknown-linux-gnuabi64");
   EXPECT_EQ(Triple::ArchType::mips64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNUABI64, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());
   T = Triple("mips64r6");
   EXPECT_EQ(Triple::ArchType::mips64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNUABI64, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());
   T = Triple("mipsisa64r6");
   EXPECT_EQ(Triple::ArchType::mips64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNUABI64, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());

   T = Triple("mips64el-unknown-linux-gnuabin32");
   EXPECT_EQ(Triple::ArchType::mips64el, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNUABIN32, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::NoSubArch, T.getSubArch());
   T = Triple("mipsn32el");
   EXPECT_EQ(Triple::ArchType::mips64el, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNUABIN32, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::NoSubArch, T.getSubArch());

   T = Triple("mips64-unknown-linux-gnuabin32");
   EXPECT_EQ(Triple::ArchType::mips64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNUABIN32, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::NoSubArch, T.getSubArch());
   T = Triple("mipsn32");
   EXPECT_EQ(Triple::ArchType::mips64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNUABIN32, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::NoSubArch, T.getSubArch());

   T = Triple("mipsisa64r6el-unknown-linux-gnuabin32");
   EXPECT_EQ(Triple::ArchType::mips64el, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNUABIN32, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());
   T = Triple("mipsn32r6el");
   EXPECT_EQ(Triple::ArchType::mips64el, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNUABIN32, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());

   T = Triple("mipsisa64r6-unknown-linux-gnuabin32");
   EXPECT_EQ(Triple::ArchType::mips64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNUABIN32, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());
   T = Triple("mipsn32r6");
   EXPECT_EQ(Triple::ArchType::mips64, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNUABIN32, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());

   T = Triple("mipsel-unknown-linux-gnu");
   EXPECT_EQ(Triple::ArchType::mipsel, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::NoSubArch, T.getSubArch());
   T = Triple("mipsel");
   EXPECT_EQ(Triple::ArchType::mipsel, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::NoSubArch, T.getSubArch());

   T = Triple("mips-unknown-linux-gnu");
   EXPECT_EQ(Triple::ArchType::mips, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::NoSubArch, T.getSubArch());
   T = Triple("mips");
   EXPECT_EQ(Triple::ArchType::mips, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::NoSubArch, T.getSubArch());

   T = Triple("mipsisa32r6el-unknown-linux-gnu");
   EXPECT_EQ(Triple::ArchType::mipsel, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());
   T = Triple("mipsr6el");
   EXPECT_EQ(Triple::ArchType::mipsel, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());
   T = Triple("mipsisa32r6el");
   EXPECT_EQ(Triple::ArchType::mipsel, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());

   T = Triple("mipsisa32r6-unknown-linux-gnu");
   EXPECT_EQ(Triple::ArchType::mips, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());
   T = Triple("mipsr6");
   EXPECT_EQ(Triple::ArchType::mips, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());
   T = Triple("mipsisa32r6");
   EXPECT_EQ(Triple::ArchType::mips, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::EnvironmentType::GNU, T.getEnvironment());
   EXPECT_EQ(Triple::SubArchType::MipsSubArch_r6, T.getSubArch());

   T = Triple("arm-oe-linux-gnueabi");
   EXPECT_EQ(Triple::ArchType::arm, T.getArch());
   EXPECT_EQ(Triple::VendorType::OpenEmbedded, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::GNUEABI, T.getEnvironment());

   T = Triple("aarch64-oe-linux");
   EXPECT_EQ(Triple::ArchType::aarch64, T.getArch());
   EXPECT_EQ(Triple::VendorType::OpenEmbedded, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T = Triple("huh");
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.getArch());
}

static std::string Join(StringRef A, StringRef B, StringRef C)
{
   std::string Str = A;
   Str += '-';
   Str += B;
   Str += '-';
   Str += C;
   return Str;
}

static std::string Join(StringRef A, StringRef B, StringRef C, StringRef D)
{
   std::string Str = A;
   Str += '-';
   Str += B;
   Str += '-';
   Str += C;
   Str += '-';
   Str += D;
   return Str;
}

TEST(TripleTest, testNormalization)
{

   EXPECT_EQ("unknown", Triple::normalize(""));
   EXPECT_EQ("unknown-unknown", Triple::normalize("-"));
   EXPECT_EQ("unknown-unknown-unknown", Triple::normalize("--"));
   EXPECT_EQ("unknown-unknown-unknown-unknown", Triple::normalize("---"));
   EXPECT_EQ("unknown-unknown-unknown-unknown-unknown",
             Triple::normalize("----"));

   EXPECT_EQ("a", Triple::normalize("a"));
   EXPECT_EQ("a-b", Triple::normalize("a-b"));
   EXPECT_EQ("a-b-c", Triple::normalize("a-b-c"));
   EXPECT_EQ("a-b-c-d", Triple::normalize("a-b-c-d"));

   EXPECT_EQ("i386-b-c", Triple::normalize("i386-b-c"));
   EXPECT_EQ("i386-a-c", Triple::normalize("a-i386-c"));
   EXPECT_EQ("i386-a-b", Triple::normalize("a-b-i386"));
   EXPECT_EQ("i386-a-b-c", Triple::normalize("a-b-c-i386"));

   EXPECT_EQ("a-pc-c", Triple::normalize("a-pc-c"));
   EXPECT_EQ("unknown-pc-b-c", Triple::normalize("pc-b-c"));
   EXPECT_EQ("a-pc-b", Triple::normalize("a-b-pc"));
   EXPECT_EQ("a-pc-b-c", Triple::normalize("a-b-c-pc"));

   EXPECT_EQ("a-b-linux", Triple::normalize("a-b-linux"));
   EXPECT_EQ("unknown-unknown-linux-b-c", Triple::normalize("linux-b-c"));
   EXPECT_EQ("a-unknown-linux-c", Triple::normalize("a-linux-c"));

   EXPECT_EQ("i386-pc-a", Triple::normalize("a-pc-i386"));
   EXPECT_EQ("i386-pc-unknown", Triple::normalize("-pc-i386"));
   EXPECT_EQ("unknown-pc-linux-c", Triple::normalize("linux-pc-c"));
   EXPECT_EQ("unknown-pc-linux", Triple::normalize("linux-pc-"));

   EXPECT_EQ("i386", Triple::normalize("i386"));
   EXPECT_EQ("unknown-pc", Triple::normalize("pc"));
   EXPECT_EQ("unknown-unknown-linux", Triple::normalize("linux"));

   EXPECT_EQ("x86_64-unknown-linux-gnu", Triple::normalize("x86_64-gnu-linux"));

   // Check that normalizing a permutated set of valid components returns a
   // triple with the unpermuted components.
   //
   // We don't check every possible combination. For the set of architectures A,
   // vendors V, operating systems O, and environments E, that would require |A|
   // * |V| * |O| * |E| * 4! tests. Instead we check every option for any given
   // slot and make sure it gets normalized to the correct position from every
   // permutation. This should cover the core logic while being a tractable
   // number of tests at (|A| + |V| + |O| + |E|) * 4!.
   auto FirstArchType = Triple::ArchType(int(Triple::ArchType::UnknownArch) + 1);
   auto FirstVendorType = Triple::VendorType(int(Triple::VendorType::UnknownVendor) + 1);
   auto FirstOSType = Triple::OSType(int(Triple::OSType::UnknownOS) + 1);
   auto FirstEnvType = Triple::EnvironmentType(int(Triple::EnvironmentType::UnknownEnvironment) + 1);
   StringRef InitialC[] = {Triple::getArchTypeName(FirstArchType),
                           Triple::getVendorTypeName(FirstVendorType),
                           Triple::getOSTypeName(FirstOSType),
                           Triple::getEnvironmentTypeName(FirstEnvType)};
   for (int Arch = (int)FirstArchType; Arch <= (int)Triple::ArchType::LastArchType; ++Arch) {
      StringRef C[] = {InitialC[0], InitialC[1], InitialC[2], InitialC[3]};
      C[0] = Triple::getArchTypeName(Triple::ArchType(Arch));
      std::string E = Join(C[0], C[1], C[2]);
      int I[] = {0, 1, 2};
      do {
         EXPECT_EQ(E, Triple::normalize(Join(C[I[0]], C[I[1]], C[I[2]])));
      } while (std::next_permutation(std::begin(I), std::end(I)));
      std::string F = Join(C[0], C[1], C[2], C[3]);
      int J[] = {0, 1, 2, 3};
      do {
         EXPECT_EQ(F, Triple::normalize(Join(C[J[0]], C[J[1]], C[J[2]], C[J[3]])));
      } while (std::next_permutation(std::begin(J), std::end(J)));
   }
   for (int Vendor = (int)FirstVendorType; Vendor <= (int)Triple::VendorType::LastVendorType;
        ++Vendor) {
      StringRef C[] = {InitialC[0], InitialC[1], InitialC[2], InitialC[3]};
      C[1] = Triple::getVendorTypeName(Triple::VendorType(Vendor));
      std::string E = Join(C[0], C[1], C[2]);
      int I[] = {0, 1, 2};
      do {
         EXPECT_EQ(E, Triple::normalize(Join(C[I[0]], C[I[1]], C[I[2]])));
      } while (std::next_permutation(std::begin(I), std::end(I)));
      std::string F = Join(C[0], C[1], C[2], C[3]);
      int J[] = {0, 1, 2, 3};
      do {
         EXPECT_EQ(F, Triple::normalize(Join(C[J[0]], C[J[1]], C[J[2]], C[J[3]])));
      } while (std::next_permutation(std::begin(J), std::end(J)));
   }
   for (int OS = (int)FirstOSType; OS <= (int)Triple::OSType::LastOSType; ++OS) {
      if (OS == (int)Triple::OSType::Win32)
         continue;
      StringRef C[] = {InitialC[0], InitialC[1], InitialC[2], InitialC[3]};
      C[2] = Triple::getOSTypeName(Triple::OSType(OS));
      std::string E = Join(C[0], C[1], C[2]);
      int I[] = {0, 1, 2};
      do {
         EXPECT_EQ(E, Triple::normalize(Join(C[I[0]], C[I[1]], C[I[2]])));
      } while (std::next_permutation(std::begin(I), std::end(I)));
      std::string F = Join(C[0], C[1], C[2], C[3]);
      int J[] = {0, 1, 2, 3};
      do {
         EXPECT_EQ(F, Triple::normalize(Join(C[J[0]], C[J[1]], C[J[2]], C[J[3]])));
      } while (std::next_permutation(std::begin(J), std::end(J)));
   }
   for (int Env = (int)FirstEnvType; Env <= (int)Triple::EnvironmentType::LastEnvironmentType; ++Env) {
      StringRef C[] = {InitialC[0], InitialC[1], InitialC[2], InitialC[3]};
      C[3] = Triple::getEnvironmentTypeName(Triple::EnvironmentType(Env));
      std::string F = Join(C[0], C[1], C[2], C[3]);
      int J[] = {0, 1, 2, 3};
      do {
         EXPECT_EQ(F, Triple::normalize(Join(C[J[0]], C[J[1]], C[J[2]], C[J[3]])));
      } while (std::next_permutation(std::begin(J), std::end(J)));
   }

   // Various real-world funky triples.  The value returned by GCC's config.sub
   // is given in the comment.
   EXPECT_EQ("i386-unknown-windows-gnu",
             Triple::normalize("i386-mingw32")); // i386-pc-mingw32
   EXPECT_EQ("x86_64-unknown-linux-gnu",
             Triple::normalize("x86_64-linux-gnu")); // x86_64-pc-linux-gnu
   EXPECT_EQ("i486-unknown-linux-gnu",
             Triple::normalize("i486-linux-gnu")); // i486-pc-linux-gnu
   EXPECT_EQ("i386-redhat-linux",
             Triple::normalize("i386-redhat-linux")); // i386-redhat-linux-gnu
   EXPECT_EQ("i686-unknown-linux",
             Triple::normalize("i686-linux")); // i686-pc-linux-gnu
   EXPECT_EQ("arm-none-unknown-eabi",
             Triple::normalize("arm-none-eabi")); // arm-none-eabi
}

TEST(TripleTest, testMutateName)
{
   Triple T;
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.getArch());
   EXPECT_EQ(Triple::VendorType::UnknownVendor, T.getVendor());
   EXPECT_EQ(Triple::OSType::UnknownOS, T.getOS());
   EXPECT_EQ(Triple::EnvironmentType::UnknownEnvironment, T.getEnvironment());

   T.setArchName("i386");
   EXPECT_EQ(Triple::ArchType::x86, T.getArch());
   EXPECT_EQ("i386--", T.getTriple());

   T.setVendorName("pc");
   EXPECT_EQ(Triple::ArchType::x86, T.getArch());
   EXPECT_EQ(Triple::VendorType::PC, T.getVendor());
   EXPECT_EQ("i386-pc-", T.getTriple());

   T.setOSName("linux");
   EXPECT_EQ(Triple::ArchType::x86, T.getArch());
   EXPECT_EQ(Triple::VendorType::PC, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ("i386-pc-linux", T.getTriple());

   T.setEnvironmentName("gnu");
   EXPECT_EQ(Triple::ArchType::x86, T.getArch());
   EXPECT_EQ(Triple::VendorType::PC, T.getVendor());
   EXPECT_EQ(Triple::OSType::Linux, T.getOS());
   EXPECT_EQ("i386-pc-linux-gnu", T.getTriple());

   T.setOSName("freebsd");
   EXPECT_EQ(Triple::ArchType::x86, T.getArch());
   EXPECT_EQ(Triple::VendorType::PC, T.getVendor());
   EXPECT_EQ(Triple::OSType::FreeBSD, T.getOS());
   EXPECT_EQ("i386-pc-freebsd-gnu", T.getTriple());

   T.setOSAndEnvironmentName("darwin");
   EXPECT_EQ(Triple::ArchType::x86, T.getArch());
   EXPECT_EQ(Triple::VendorType::PC, T.getVendor());
   EXPECT_EQ(Triple::OSType::Darwin, T.getOS());
   EXPECT_EQ("i386-pc-darwin", T.getTriple());
}

TEST(TripleTest, testBitWidthPredicates)
{
   Triple T;
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::arm);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::hexagon);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::mips);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::mips64);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_TRUE(T.isArch64Bit());

   T.setArch(Triple::ArchType::msp430);
   EXPECT_TRUE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::ppc);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::ppc64);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_TRUE(T.isArch64Bit());

   T.setArch(Triple::ArchType::x86);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::x86_64);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_TRUE(T.isArch64Bit());

   T.setArch(Triple::ArchType::amdil);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::amdil64);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_TRUE(T.isArch64Bit());

   T.setArch(Triple::ArchType::hsail);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::hsail64);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_TRUE(T.isArch64Bit());

   T.setArch(Triple::ArchType::spir);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::spir64);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_TRUE(T.isArch64Bit());

   T.setArch(Triple::ArchType::sparc);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::sparcel);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::sparcv9);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_TRUE(T.isArch64Bit());

   T.setArch(Triple::ArchType::wasm32);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::wasm64);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_TRUE(T.isArch64Bit());

   T.setArch(Triple::ArchType::avr);
   EXPECT_TRUE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::lanai);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::riscv32);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());

   T.setArch(Triple::ArchType::riscv64);
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_TRUE(T.isArch64Bit());
}

TEST(TripleTest, testBitWidthArchVariants)
{
   Triple T;
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::UnknownArch);
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::mips);
   EXPECT_EQ(Triple::ArchType::mips, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::mips64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::mipsel);
   EXPECT_EQ(Triple::ArchType::mipsel, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::mips64el, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::ppc);
   EXPECT_EQ(Triple::ArchType::ppc, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::ppc64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::nvptx);
   EXPECT_EQ(Triple::ArchType::nvptx, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::nvptx64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::sparc);
   EXPECT_EQ(Triple::ArchType::sparc, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::sparcv9, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::x86);
   EXPECT_EQ(Triple::ArchType::x86, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::x86_64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::mips64);
   EXPECT_EQ(Triple::ArchType::mips, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::mips64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::mips64el);
   EXPECT_EQ(Triple::ArchType::mipsel, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::mips64el, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::ppc64);
   EXPECT_EQ(Triple::ArchType::ppc, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::ppc64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::nvptx64);
   EXPECT_EQ(Triple::ArchType::nvptx, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::nvptx64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::sparcv9);
   EXPECT_EQ(Triple::ArchType::sparc, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::sparcv9, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::x86_64);
   EXPECT_EQ(Triple::ArchType::x86, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::x86_64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::amdil);
   EXPECT_EQ(Triple::ArchType::amdil, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::amdil64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::amdil64);
   EXPECT_EQ(Triple::ArchType::amdil, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::amdil64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::hsail);
   EXPECT_EQ(Triple::ArchType::hsail, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::hsail64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::hsail64);
   EXPECT_EQ(Triple::ArchType::hsail, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::hsail64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::spir);
   EXPECT_EQ(Triple::ArchType::spir, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::spir64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::spir64);
   EXPECT_EQ(Triple::ArchType::spir, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::spir64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::wasm32);
   EXPECT_EQ(Triple::ArchType::wasm32, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::wasm64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::wasm64);
   EXPECT_EQ(Triple::ArchType::wasm32, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::wasm64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::riscv32);
   EXPECT_EQ(Triple::ArchType::riscv32, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::riscv64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::riscv64);
   EXPECT_EQ(Triple::ArchType::riscv32, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::riscv64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::thumbeb);
   EXPECT_EQ(Triple::ArchType::thumbeb, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::aarch64_be, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::thumb);
   EXPECT_EQ(Triple::ArchType::thumb, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::aarch64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::aarch64);
   EXPECT_EQ(Triple::ArchType::arm, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::aarch64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::aarch64_be);
   EXPECT_EQ(Triple::ArchType::armeb, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::aarch64_be, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::renderscript32);
   EXPECT_EQ(Triple::ArchType::renderscript32, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::renderscript64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::renderscript64);
   EXPECT_EQ(Triple::ArchType::renderscript32, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::renderscript64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::le32);
   EXPECT_EQ(Triple::ArchType::le32, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::le64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::le64);
   EXPECT_EQ(Triple::ArchType::le32, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::le64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::armeb);
   EXPECT_EQ(Triple::ArchType::armeb, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::aarch64_be, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::arm);
   EXPECT_EQ(Triple::ArchType::arm, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::aarch64, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::systemz);
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::systemz, T.get64BitArchVariant().getArch());

   T.setArch(Triple::ArchType::xcore);
   EXPECT_EQ(Triple::ArchType::xcore, T.get32BitArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.get64BitArchVariant().getArch());
}

TEST(TripleTest, testEndianArchVariants)
{
   Triple T;
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::UnknownArch);
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::aarch64_be);
   EXPECT_EQ(Triple::ArchType::aarch64_be, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::aarch64, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::aarch64);
   EXPECT_EQ(Triple::ArchType::aarch64_be, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::aarch64, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::armeb);
   EXPECT_EQ(Triple::ArchType::armeb, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::arm);
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::arm, T.getLittleEndianArchVariant().getArch());
   T = Triple("arm");
   EXPECT_TRUE(T.isLittleEndian());
   T = Triple("thumb");
   EXPECT_TRUE(T.isLittleEndian());
   T = Triple("armeb");
   EXPECT_FALSE(T.isLittleEndian());
   T = Triple("thumbeb");
   EXPECT_FALSE(T.isLittleEndian());

   T.setArch(Triple::ArchType::bpfeb);
   EXPECT_EQ(Triple::ArchType::bpfeb, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::bpfel, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::bpfel);
   EXPECT_EQ(Triple::ArchType::bpfeb, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::bpfel, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::mips64);
   EXPECT_EQ(Triple::ArchType::mips64, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::mips64el, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::mips64el);
   EXPECT_EQ(Triple::ArchType::mips64, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::mips64el, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::mips);
   EXPECT_EQ(Triple::ArchType::mips, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::mipsel, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::mipsel);
   EXPECT_EQ(Triple::ArchType::mips, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::mipsel, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::ppc);
   EXPECT_EQ(Triple::ArchType::ppc, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::ppc64);
   EXPECT_EQ(Triple::ArchType::ppc64, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::ppc64le, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::ppc64le);
   EXPECT_EQ(Triple::ArchType::ppc64, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::ppc64le, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::sparc);
   EXPECT_EQ(Triple::ArchType::sparc, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::sparcel, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::sparcel);
   EXPECT_EQ(Triple::ArchType::sparc, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::sparcel, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::thumb);
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::thumb, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::thumbeb);
   EXPECT_EQ(Triple::ArchType::thumbeb, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::lanai);
   EXPECT_EQ(Triple::ArchType::lanai, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::tcele);
   EXPECT_EQ(Triple::ArchType::tce, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::tcele, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::tce);
   EXPECT_EQ(Triple::ArchType::tce, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::tcele, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::le32);
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::le32, T.getLittleEndianArchVariant().getArch());

   T.setArch(Triple::ArchType::le64);
   EXPECT_EQ(Triple::ArchType::UnknownArch, T.getBigEndianArchVariant().getArch());
   EXPECT_EQ(Triple::ArchType::le64, T.getLittleEndianArchVariant().getArch());
}

TEST(TripleTest, testGetOSVersion)
{
   Triple T;
   unsigned Major, Minor, Micro;

   T = Triple("i386-apple-darwin9");
   EXPECT_TRUE(T.isMacOSX());
   EXPECT_FALSE(T.isiOS());
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());
   T.getMacOSXVersion(Major, Minor, Micro);
   EXPECT_EQ((unsigned)10, Major);
   EXPECT_EQ((unsigned)5, Minor);
   EXPECT_EQ((unsigned)0, Micro);
   T.getiOSVersion(Major, Minor, Micro);
   EXPECT_EQ((unsigned)5, Major);
   EXPECT_EQ((unsigned)0, Minor);
   EXPECT_EQ((unsigned)0, Micro);

   T = Triple("x86_64-apple-darwin9");
   EXPECT_TRUE(T.isMacOSX());
   EXPECT_FALSE(T.isiOS());
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_TRUE(T.isArch64Bit());
   T.getMacOSXVersion(Major, Minor, Micro);
   EXPECT_EQ((unsigned)10, Major);
   EXPECT_EQ((unsigned)5, Minor);
   EXPECT_EQ((unsigned)0, Micro);
   T.getiOSVersion(Major, Minor, Micro);
   EXPECT_EQ((unsigned)5, Major);
   EXPECT_EQ((unsigned)0, Minor);
   EXPECT_EQ((unsigned)0, Micro);

   T = Triple("x86_64-apple-macosx");
   EXPECT_TRUE(T.isMacOSX());
   EXPECT_FALSE(T.isiOS());
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_TRUE(T.isArch64Bit());
   T.getMacOSXVersion(Major, Minor, Micro);
   EXPECT_EQ((unsigned)10, Major);
   EXPECT_EQ((unsigned)4, Minor);
   EXPECT_EQ((unsigned)0, Micro);
   T.getiOSVersion(Major, Minor, Micro);
   EXPECT_EQ((unsigned)5, Major);
   EXPECT_EQ((unsigned)0, Minor);
   EXPECT_EQ((unsigned)0, Micro);

   T = Triple("x86_64-apple-macosx10.7");
   EXPECT_TRUE(T.isMacOSX());
   EXPECT_FALSE(T.isiOS());
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_FALSE(T.isArch32Bit());
   EXPECT_TRUE(T.isArch64Bit());
   T.getMacOSXVersion(Major, Minor, Micro);
   EXPECT_EQ((unsigned)10, Major);
   EXPECT_EQ((unsigned)7, Minor);
   EXPECT_EQ((unsigned)0, Micro);
   T.getiOSVersion(Major, Minor, Micro);
   EXPECT_EQ((unsigned)5, Major);
   EXPECT_EQ((unsigned)0, Minor);
   EXPECT_EQ((unsigned)0, Micro);

   T = Triple("armv7-apple-ios");
   EXPECT_FALSE(T.isMacOSX());
   EXPECT_TRUE(T.isiOS());
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());
   T.getMacOSXVersion(Major, Minor, Micro);
   EXPECT_EQ((unsigned)10, Major);
   EXPECT_EQ((unsigned)4, Minor);
   EXPECT_EQ((unsigned)0, Micro);
   T.getiOSVersion(Major, Minor, Micro);
   EXPECT_EQ((unsigned)5, Major);
   EXPECT_EQ((unsigned)0, Minor);
   EXPECT_EQ((unsigned)0, Micro);

   T = Triple("armv7-apple-ios7.0");
   EXPECT_FALSE(T.isMacOSX());
   EXPECT_TRUE(T.isiOS());
   EXPECT_FALSE(T.isArch16Bit());
   EXPECT_TRUE(T.isArch32Bit());
   EXPECT_FALSE(T.isArch64Bit());
   T.getMacOSXVersion(Major, Minor, Micro);
   EXPECT_EQ((unsigned)10, Major);
   EXPECT_EQ((unsigned)4, Minor);
   EXPECT_EQ((unsigned)0, Micro);
   T.getiOSVersion(Major, Minor, Micro);
   EXPECT_EQ((unsigned)7, Major);
   EXPECT_EQ((unsigned)0, Minor);
   EXPECT_EQ((unsigned)0, Micro);
   EXPECT_FALSE(T.isSimulatorEnvironment());

   T = Triple("x86_64-apple-ios10.3-simulator");
   EXPECT_TRUE(T.isiOS());
   T.getiOSVersion(Major, Minor, Micro);
   EXPECT_EQ((unsigned)10, Major);
   EXPECT_EQ((unsigned)3, Minor);
   EXPECT_EQ((unsigned)0, Micro);
   EXPECT_TRUE(T.isSimulatorEnvironment());
}

TEST(TripleTest, testFileFormat)
{
   EXPECT_EQ(Triple::ObjectFormatType::ELF, Triple("i686-unknown-linux-gnu").getObjectFormat());
   EXPECT_EQ(Triple::ObjectFormatType::ELF, Triple("i686-unknown-freebsd").getObjectFormat());
   EXPECT_EQ(Triple::ObjectFormatType::ELF, Triple("i686-unknown-netbsd").getObjectFormat());
   EXPECT_EQ(Triple::ObjectFormatType::ELF, Triple("i686--win32-elf").getObjectFormat());
   EXPECT_EQ(Triple::ObjectFormatType::ELF, Triple("i686---elf").getObjectFormat());

   EXPECT_EQ(Triple::ObjectFormatType::MachO, Triple("i686-apple-macosx").getObjectFormat());
   EXPECT_EQ(Triple::ObjectFormatType::MachO, Triple("i686-apple-ios").getObjectFormat());
   EXPECT_EQ(Triple::ObjectFormatType::MachO, Triple("i686---macho").getObjectFormat());

   EXPECT_EQ(Triple::ObjectFormatType::COFF, Triple("i686--win32").getObjectFormat());

   EXPECT_EQ(Triple::ObjectFormatType::ELF, Triple("i686-pc-windows-msvc-elf").getObjectFormat());
   EXPECT_EQ(Triple::ObjectFormatType::ELF, Triple("i686-pc-cygwin-elf").getObjectFormat());

   EXPECT_EQ(Triple::ObjectFormatType::Wasm, Triple("wasm32-unknown-unknown").getObjectFormat());
   EXPECT_EQ(Triple::ObjectFormatType::Wasm, Triple("wasm64-unknown-unknown").getObjectFormat());

   EXPECT_EQ(Triple::ObjectFormatType::Wasm,
             Triple("wasm32-unknown-unknown-wasm").getObjectFormat());
   EXPECT_EQ(Triple::ObjectFormatType::Wasm,
             Triple("wasm64-unknown-unknown-wasm").getObjectFormat());

   Triple MSVCNormalized(Triple::normalize("i686-pc-windows-msvc-elf"));
   EXPECT_EQ(Triple::ObjectFormatType::ELF, MSVCNormalized.getObjectFormat());

   Triple GNUWindowsNormalized(Triple::normalize("i686-pc-windows-gnu-elf"));
   EXPECT_EQ(Triple::ObjectFormatType::ELF, GNUWindowsNormalized.getObjectFormat());

   Triple CygnusNormalised(Triple::normalize("i686-pc-windows-cygnus-elf"));
   EXPECT_EQ(Triple::ObjectFormatType::ELF, CygnusNormalised.getObjectFormat());

   Triple CygwinNormalized(Triple::normalize("i686-pc-cygwin-elf"));
   EXPECT_EQ(Triple::ObjectFormatType::ELF, CygwinNormalized.getObjectFormat());

   Triple T = Triple("");
   T.setObjectFormat(Triple::ObjectFormatType::ELF);
   EXPECT_EQ(Triple::ObjectFormatType::ELF, T.getObjectFormat());

   T.setObjectFormat(Triple::ObjectFormatType::MachO);
   EXPECT_EQ(Triple::ObjectFormatType::MachO, T.getObjectFormat());
}

TEST(TripleTest, testNormalizeWindows)
{
   EXPECT_EQ("i686-pc-windows-msvc", Triple::normalize("i686-pc-win32"));
   EXPECT_EQ("i686-unknown-windows-msvc", Triple::normalize("i686-win32"));
   EXPECT_EQ("i686-pc-windows-gnu", Triple::normalize("i686-pc-mingw32"));
   EXPECT_EQ("i686-unknown-windows-gnu", Triple::normalize("i686-mingw32"));
   EXPECT_EQ("i686-pc-windows-gnu", Triple::normalize("i686-pc-mingw32-w64"));
   EXPECT_EQ("i686-unknown-windows-gnu", Triple::normalize("i686-mingw32-w64"));
   EXPECT_EQ("i686-pc-windows-cygnus", Triple::normalize("i686-pc-cygwin"));
   EXPECT_EQ("i686-unknown-windows-cygnus", Triple::normalize("i686-cygwin"));

   EXPECT_EQ("x86_64-pc-windows-msvc", Triple::normalize("x86_64-pc-win32"));
   EXPECT_EQ("x86_64-unknown-windows-msvc", Triple::normalize("x86_64-win32"));
   EXPECT_EQ("x86_64-pc-windows-gnu", Triple::normalize("x86_64-pc-mingw32"));
   EXPECT_EQ("x86_64-unknown-windows-gnu", Triple::normalize("x86_64-mingw32"));
   EXPECT_EQ("x86_64-pc-windows-gnu",
             Triple::normalize("x86_64-pc-mingw32-w64"));
   EXPECT_EQ("x86_64-unknown-windows-gnu",
             Triple::normalize("x86_64-mingw32-w64"));

   EXPECT_EQ("i686-pc-windows-elf", Triple::normalize("i686-pc-win32-elf"));
   EXPECT_EQ("i686-unknown-windows-elf", Triple::normalize("i686-win32-elf"));
   EXPECT_EQ("i686-pc-windows-macho", Triple::normalize("i686-pc-win32-macho"));
   EXPECT_EQ("i686-unknown-windows-macho",
             Triple::normalize("i686-win32-macho"));

   EXPECT_EQ("x86_64-pc-windows-elf", Triple::normalize("x86_64-pc-win32-elf"));
   EXPECT_EQ("x86_64-unknown-windows-elf",
             Triple::normalize("x86_64-win32-elf"));
   EXPECT_EQ("x86_64-pc-windows-macho",
             Triple::normalize("x86_64-pc-win32-macho"));
   EXPECT_EQ("x86_64-unknown-windows-macho",
             Triple::normalize("x86_64-win32-macho"));

   EXPECT_EQ("i686-pc-windows-cygnus",
             Triple::normalize("i686-pc-windows-cygnus"));
   EXPECT_EQ("i686-pc-windows-gnu", Triple::normalize("i686-pc-windows-gnu"));
   EXPECT_EQ("i686-pc-windows-itanium",
             Triple::normalize("i686-pc-windows-itanium"));
   EXPECT_EQ("i686-pc-windows-msvc", Triple::normalize("i686-pc-windows-msvc"));

   EXPECT_EQ("i686-pc-windows-elf",
             Triple::normalize("i686-pc-windows-elf-elf"));
}

TEST(TripleTest, testGetARMCPUForArch)
{
   // Platform specific defaults.
   {
      polar::basic::Triple Triple("arm--nacl");
      EXPECT_EQ("cortex-a8", Triple.getARMCPUForArch());
   }
   {
      polar::basic::Triple Triple("arm--openbsd");
      EXPECT_EQ("cortex-a8", Triple.getARMCPUForArch());
   }
   {
      polar::basic::Triple Triple("armv6-unknown-freebsd");
      EXPECT_EQ("arm1176jzf-s", Triple.getARMCPUForArch());
   }
   {
      polar::basic::Triple Triple("thumbv6-unknown-freebsd");
      EXPECT_EQ("arm1176jzf-s", Triple.getARMCPUForArch());
   }
   {
      polar::basic::Triple Triple("armebv6-unknown-freebsd");
      EXPECT_EQ("arm1176jzf-s", Triple.getARMCPUForArch());
   }
   {
      polar::basic::Triple Triple("arm--win32");
      EXPECT_EQ("cortex-a9", Triple.getARMCPUForArch());
   }
   // Some alternative architectures
   {
      polar::basic::Triple Triple("armv7k-apple-ios9");
      EXPECT_EQ("cortex-a7", Triple.getARMCPUForArch());
   }
   {
      polar::basic::Triple Triple("armv7k-apple-watchos3");
      EXPECT_EQ("cortex-a7", Triple.getARMCPUForArch());
   }
   {
      polar::basic::Triple Triple("armv7k-apple-tvos9");
      EXPECT_EQ("cortex-a7", Triple.getARMCPUForArch());
   }
   // armeb is permitted, but armebeb is not
   {
      polar::basic::Triple Triple("armeb-none-eabi");
      EXPECT_EQ("arm7tdmi", Triple.getARMCPUForArch());
   }
   {
      polar::basic::Triple Triple("armebeb-none-eabi");
      EXPECT_EQ("", Triple.getARMCPUForArch());
   }
   {
      polar::basic::Triple Triple("armebv6eb-none-eabi");
      EXPECT_EQ("", Triple.getARMCPUForArch());
   }
   // xscaleeb is permitted, but armebxscale is not
   {
      polar::basic::Triple Triple("xscaleeb-none-eabi");
      EXPECT_EQ("xscale", Triple.getARMCPUForArch());
   }
   {
      polar::basic::Triple Triple("armebxscale-none-eabi");
      EXPECT_EQ("", Triple.getARMCPUForArch());
   }
}

TEST(TripleTest, testNormalizeARM)
{
   EXPECT_EQ("armv6-unknown-netbsd-eabi",
             Triple::normalize("armv6-netbsd-eabi"));
   EXPECT_EQ("armv7-unknown-netbsd-eabi",
             Triple::normalize("armv7-netbsd-eabi"));
   EXPECT_EQ("armv6eb-unknown-netbsd-eabi",
             Triple::normalize("armv6eb-netbsd-eabi"));
   EXPECT_EQ("armv7eb-unknown-netbsd-eabi",
             Triple::normalize("armv7eb-netbsd-eabi"));
   EXPECT_EQ("armv6-unknown-netbsd-eabihf",
             Triple::normalize("armv6-netbsd-eabihf"));
   EXPECT_EQ("armv7-unknown-netbsd-eabihf",
             Triple::normalize("armv7-netbsd-eabihf"));
   EXPECT_EQ("armv6eb-unknown-netbsd-eabihf",
             Triple::normalize("armv6eb-netbsd-eabihf"));
   EXPECT_EQ("armv7eb-unknown-netbsd-eabihf",
             Triple::normalize("armv7eb-netbsd-eabihf"));

   EXPECT_EQ("armv7-suse-linux-gnueabihf",
             Triple::normalize("armv7-suse-linux-gnueabi"));

   Triple T;
   T = Triple("armv6--netbsd-eabi");
   EXPECT_EQ(Triple::ArchType::arm, T.getArch());
   T = Triple("armv6eb--netbsd-eabi");
   EXPECT_EQ(Triple::ArchType::armeb, T.getArch());
   T = Triple("armv7-suse-linux-gnueabihf");
   EXPECT_EQ(Triple::EnvironmentType::GNUEABIHF, T.getEnvironment());
}

TEST(TripleTest, testParseARMArch)
{
   // ARM
   {
      Triple T = Triple("arm");
      EXPECT_EQ(Triple::ArchType::arm, T.getArch());
   }
   {
      Triple T = Triple("armeb");
      EXPECT_EQ(Triple::ArchType::armeb, T.getArch());
   }
   // THUMB
   {
      Triple T = Triple("thumb");
      EXPECT_EQ(Triple::ArchType::thumb, T.getArch());
   }
   {
      Triple T = Triple("thumbeb");
      EXPECT_EQ(Triple::ArchType::thumbeb, T.getArch());
   }
   // AARCH64
   {
      Triple T = Triple("arm64");
      EXPECT_EQ(Triple::ArchType::aarch64, T.getArch());
   }
   {
      Triple T = Triple("aarch64");
      EXPECT_EQ(Triple::ArchType::aarch64, T.getArch());
   }
   {
      Triple T = Triple("aarch64_be");
      EXPECT_EQ(Triple::ArchType::aarch64_be, T.getArch());
   }
}

} // anonymous namespace
