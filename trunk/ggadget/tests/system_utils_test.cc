/*
  Copyright 2007 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include "ggadget/logger.h"
#include "ggadget/system_utils.h"
#include "ggadget/gadget_consts.h"
#include "unittest/gunit.h"

using namespace ggadget;

TEST(SystemUtils, BuildPath) {
  EXPECT_STREQ(
      "/abc/def/ghi",
      BuildPath(kDirSeparatorStr, "/", "/abc", "def/", "ghi", NULL).c_str());
  EXPECT_STREQ(
      "hello/:world",
      BuildPath("/:", "hello", "", "world", NULL).c_str());
  EXPECT_STREQ(
      "hello",
      BuildPath("//", "hello", NULL).c_str());
  EXPECT_STREQ(
      "/usr/sbin/sudo",
      BuildPath(kDirSeparatorStr, "//usr", "sbin//", "//sudo", NULL).c_str());
  EXPECT_STREQ(
      "//usr//sbin//a//sudo",
      BuildPath("//", "//usr", "//", "sbin", "////a//", "sudo", NULL).c_str());
  EXPECT_STREQ(
      "//usr",
      BuildPath("//", "////", "//////", "usr//", "////", "////", NULL).c_str());
}


TEST(SystemUtils, SplitFilePath) {
  std::string dir;
  std::string file;
  EXPECT_TRUE(SplitFilePath("/foo/bar/file", &dir, &file));
  EXPECT_STREQ("/foo/bar", dir.c_str());
  EXPECT_STREQ("file", file.c_str());
  EXPECT_FALSE(SplitFilePath("file", &dir, &file));
  EXPECT_STREQ("", dir.c_str());
  EXPECT_STREQ("file", file.c_str());
  EXPECT_FALSE(SplitFilePath("dir/", &dir, &file));
  EXPECT_STREQ("dir", dir.c_str());
  EXPECT_STREQ("", file.c_str());
  EXPECT_TRUE(SplitFilePath("dir///file", &dir, &file));
  EXPECT_STREQ("dir", dir.c_str());
  EXPECT_STREQ("file", file.c_str());
  EXPECT_TRUE(SplitFilePath("///dir///file", &dir, &file));
  EXPECT_STREQ("///dir", dir.c_str());
  EXPECT_STREQ("file", file.c_str());
}

TEST(SystemUtils, EnsureDirectories) {
#define TEST_HOME "/tmp/TestEnsureDirectories"
  EXPECT_FALSE(EnsureDirectories(""));
  // NOTE: The following tests are Unix/Linux specific.
  EXPECT_TRUE(EnsureDirectories("/etc"));
  EXPECT_FALSE(EnsureDirectories("/etc/hosts"));
  EXPECT_FALSE(EnsureDirectories("/etc/hosts/anything"));
  EXPECT_TRUE(EnsureDirectories("/tmp"));
  EXPECT_TRUE(EnsureDirectories("/tmp/"));
  system("rm -rf " TEST_HOME);
  EXPECT_TRUE(EnsureDirectories(TEST_HOME));
  system("rm -rf " TEST_HOME);
  EXPECT_TRUE(EnsureDirectories(TEST_HOME "/"));
  EXPECT_TRUE(EnsureDirectories(TEST_HOME "/a/b/c/d/e"));
  system("touch " TEST_HOME "/file");
  EXPECT_FALSE(EnsureDirectories(TEST_HOME "/file"));
  EXPECT_FALSE(EnsureDirectories(TEST_HOME "/file/"));
  EXPECT_FALSE(EnsureDirectories(TEST_HOME "/file/a/b/c"));

  char cwd[4096];
  ASSERT_TRUE(getcwd(cwd, sizeof(cwd)));
  chdir(TEST_HOME);
  EXPECT_TRUE(EnsureDirectories("a/b/c/d/e"));
  EXPECT_TRUE(EnsureDirectories("d/e"));
  chdir(cwd);  
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
