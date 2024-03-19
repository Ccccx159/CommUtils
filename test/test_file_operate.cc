#include "commutils/commutils.h"
#include "gtest/gtest.h"
#include "test.h"

#include <dirent.h>

class GlobalEvent : public testing::Environment {
 public:
  void SetUp() override { LOG(INFO) << "global before init"; }
  void TearDown() override { LOG(INFO) << "global after deinit"; }
};

TEST(TestFileCreate, NewFile) {
  std::string f = "./.TempFileCreateEmpty";
  std::string cmd = "rm -rf " + f;
  system(cmd.c_str());
  EXPECT_EQ(0, FileOperate().CreateEmpty(f));
  EXPECT_TRUE(FileOperate().IsExist(f));
  system(cmd.c_str());
}

TEST(TestFileCreate, ExistedFile) {
  std::string f = "./.TempFileCreateEmpty";
  std::string cmd = "touch " + f;
  system(cmd.c_str());
  EXPECT_EQ(-1, FileOperate().CreateEmpty(f));
  cmd = "rm -rf " + f;
  system(cmd.c_str());
}

class FileOperateTest : public testing::Test {
 protected:
  void SetUp() override {
    f.MkdirsByPath(dir);
    if (0 != f.CreateEmpty(dir + fname)) {
      throw std::runtime_error("Create file failed");
    }
  }
  void TearDown() override {
    if (true == f.IsExist(dir + fname)) {
      std::string cmd = "rm -rf " + dir + fname;
      system(cmd.c_str());
    }
  }

 protected:
  json test_json = {
      {"name", "John"},
      {"native",
       {{"country", "China"}, {"province", "Zhejiang"}, {"city", "Hangzhou"}}},
      {"sex", "male"},
      {"age", 18}};
  std::string test_string = "This is a test string";
  std::string fname = ".tmp_FileOperateTest";
  std::string dir = "./.test/";
  FileOperate f;
};

TEST_F(FileOperateTest, CheckFileExist) {
  EXPECT_TRUE(f.IsExist(dir + fname));
}

TEST_F(FileOperateTest, CheckFileNotExist) {
  EXPECT_FALSE(f.IsExist(fname));
}

TEST_F(FileOperateTest, WriteJsonToFile) {
  EXPECT_EQ(test_json.dump().length(), f.WriteTo(dir + fname, test_json));
  json read_json;
  EXPECT_EQ(test_json.dump().length(), f.ReadFrom(dir + fname, read_json));
  EXPECT_EQ(test_json, read_json);
}

TEST_F(FileOperateTest, WriteStringToFile) {
  EXPECT_EQ(test_string.length(), f.WriteTo(dir + fname, test_string));
  std::string read_string;
  EXPECT_EQ(test_string.length(), f.ReadFrom(dir + fname, read_string));
  EXPECT_EQ(test_string, read_string);
}

TEST_F(FileOperateTest, SearchFileWithRegex) {
  std::string fp;
  std::string regex = ".*tmp.*";
  std::string pattern = "regex";
  EXPECT_TRUE(f.SearchRecursively(fp, regex, dir, 1, pattern));
  EXPECT_EQ(dir + fname, fp);
}

TEST_F(FileOperateTest, SearchFileWithRegexNotExisted) {
  std::string fp = "";
  std::string regex = ".*abc.*";
  std::string pattern = "regex";
  EXPECT_FALSE(f.SearchRecursively(fp, regex, dir, 1, pattern));
  EXPECT_EQ("", fp);
}

TEST_F(FileOperateTest, SearchFileWithRegexRecursively) {
  std::string fp;
  std::string regex = ".*tmp.*";
  std::string path = "./";
  int depth = 2;
  std::string pattern = "regex";
  EXPECT_TRUE(f.SearchRecursively(fp, regex, path, depth, pattern));
  EXPECT_EQ(dir + fname, fp);
}

TEST_F(FileOperateTest, SearchFileWithKeyword) {
  std::string fp;
  std::string keyword = "tmp";
  std::string pattern = "keyword";
  EXPECT_TRUE(f.SearchRecursively(fp, keyword, dir, 1, pattern));
  EXPECT_EQ(dir + fname, fp);
}

TEST_F(FileOperateTest, SearchFileWithKeywordNotExisted) {
  std::string fp;
  std::string keyword = "abc";
  std::string pattern = "keyword";
  EXPECT_FALSE(f.SearchRecursively(fp, keyword, dir, 1, pattern));
}

TEST_F(FileOperateTest, SearchFileWithKeywordRecursively) {
  std::string fp;
  std::string keyword = "tmp";
  std::string path = "./";
  int depth = 2;
  std::string pattern = "keyword";
  EXPECT_TRUE(f.SearchRecursively(fp, keyword, path, depth, pattern));
  EXPECT_EQ(dir + fname, fp);
}

TEST_F(FileOperateTest, GetFileNameWithoutSuffix) {
  std::string file = "test.txt";
  std::string name = "test";
  EXPECT_EQ(name, f.GetNameWithoutSuffix(file));
  EXPECT_EQ(fname, f.GetNameWithoutSuffix(dir + fname));
}

TEST_F(FileOperateTest, GetFileSuffix) {
  std::string file = "test.txt";
  std::string suffix = "txt";
  EXPECT_EQ(suffix, f.GetSuffix(file));
  EXPECT_EQ("", f.GetSuffix(fname));
}

TEST_F(FileOperateTest, ListFilesInDir) {
  std::vector<std::string> fl = f.ListInDir(dir, "file", "");
  EXPECT_EQ(1, fl.size());
  EXPECT_EQ(dir + fname, fl[0]);
}

int main(int argc, char **argv) {
  setLogger();
  testing::AddGlobalTestEnvironment(new GlobalEvent);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}