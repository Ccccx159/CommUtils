#include "commutils/file_.h"
#include "gtest/gtest.h"
#include <cstdio>

class FileEdges : public testing::Test {
 protected:
  std::string dir = "commutils_file_edges_" + std::to_string(getpid());
  void TearDown() override {
    std::remove((dir + "/data").c_str());
    rmdir((dir + "/nested/deep").c_str());
    rmdir((dir + "/nested").c_str());
    rmdir(dir.c_str());
  }
};

TEST_F(FileEdges, CreatesRelativePathWithoutSlashAndNestedDirectories) {
  FileOperate files;
  EXPECT_EQ(files.MkdirsByPath(dir), 0);
  EXPECT_EQ(files.MkdirsByPath(dir + "/nested/deep///"), 0);
  EXPECT_EQ(files.MkdirsByPath(dir + "/nested/deep"), 0);
  EXPECT_EQ(files.MkdirsByPath("/"), 0);
  EXPECT_EQ(files.MkdirsByPath(""), -1);
  ASSERT_EQ(files.CreateEmpty(dir + "/data"), 0);
  EXPECT_EQ(files.MkdirsByPath(dir + "/data"), -1);
  EXPECT_EQ(files.MkdirsByPath(dir + "/data/child"), -1);
}

TEST_F(FileEdges, HonorsAppendAndBinaryModesAndReadsEmptyFiles) {
  FileOperate files;
  ASSERT_EQ(files.MkdirsByPath(dir), 0);
  const std::string path = dir + "/data";
  ASSERT_EQ(files.CreateEmpty(path), 0);
  std::string data = "old";
  EXPECT_EQ(files.ReadFrom(path, data), 0);
  EXPECT_TRUE(data.empty());
  EXPECT_EQ(files.ReadFrom(dir, data), -1);
  EXPECT_EQ(files.ReadFrom(dir + "/missing", data), -1);
  const std::string binary("a\0b", 3);
  ASSERT_EQ(files.WriteTo(path, binary, std::ios::out | std::ios::binary), 3);
  EXPECT_EQ(files.WriteTo(path, "tail", std::ios::out | std::ios::app), 7);
  ASSERT_EQ(files.ReadFrom(path, data, std::ios::in | std::ios::binary), 7);
  EXPECT_EQ(data, binary + "tail");
}

TEST_F(FileEdges, HandlesEmptyPathsAndFiltersDirectories) {
  FileOperate files;
  std::string found;
  EXPECT_FALSE(files.SearchRecursively(found, "x", ""));
  EXPECT_TRUE(files.GetSuffix("").empty());
  EXPECT_TRUE(files.GetNameWithoutSuffix("").empty());
  EXPECT_TRUE(files.GetSuffix("/").empty());
  ASSERT_EQ(files.MkdirsByPath(dir + "/nested"), 0);
  ASSERT_EQ(files.CreateEmpty(dir + "/data"), 0);
  const auto dirs = files.ListInDir(dir, "dir");
  ASSERT_EQ(dirs.size(), 1U);
  EXPECT_EQ(dirs[0], dir + "/nested");
  const auto entries = files.ListInDir(dir, "file");
  ASSERT_EQ(entries.size(), 1U);
  EXPECT_EQ(entries[0], dir + "/data");
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
