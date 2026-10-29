#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <vector>

#include <SpiceUsr.h>

#include <SpiceQL/daf_reader.h>
#include <SpiceQL/utils.h>

#include "Fixtures.h"

using namespace SpiceQL;


namespace {

  // ckcov_c's INTERVAL level window for one ID, in the given time system
  daf::Window cspiceCkCoverage(const std::string &path, int id, const std::string &timsys) {
    SPICEDOUBLE_CELL(cover, 20000);
    scard_c(0, &cover);
    ckcov_c(path.c_str(), id, SPICEFALSE, "INTERVAL", 0.0, timsys.c_str(), &cover);
    checkNaifErrors();
    daf::Window w;
    for (SpiceInt i = 0; i < wncard_c(&cover); i++) {
      double b, e;
      wnfetd_c(&cover, i, &b, &e);
      w.emplace_back(b, e);
    }
    return w;
  }

  daf::Window cspiceSpkCoverage(const std::string &path, int id) {
    SPICEDOUBLE_CELL(cover, 20000);
    scard_c(0, &cover);
    spkcov_c(path.c_str(), id, &cover);
    checkNaifErrors();
    daf::Window w;
    for (SpiceInt i = 0; i < wncard_c(&cover); i++) {
      double b, e;
      wnfetd_c(&cover, i, &b, &e);
      w.emplace_back(b, e);
    }
    return w;
  }

  // Every ID's merged coverage, the way readDafCoverage builds it
  std::map<int, daf::Window> ckCoverageTicks(const daf::File &f) {
    std::map<int, daf::Window> cover;
    for (const daf::Summary &s : f.summaries()) {
      daf::Window w = daf::ckSegmentCoverage(f, s);
      daf::Window &dst = cover[s.ic[0]];
      dst.insert(dst.end(), w.begin(), w.end());
    }
    for (auto &[id, w] : cover) w = daf::merge(std::move(w));
    return cover;
  }

  std::map<int, daf::Window> spkCoverage(const daf::File &f) {
    std::map<int, daf::Window> cover;
    for (const daf::Summary &s : f.summaries()) {
      cover[s.ic[0]].emplace_back(s.dc[0], s.dc[1]);
    }
    for (auto &[id, w] : cover) w = daf::merge(std::move(w));
    return cover;
  }

  std::vector<std::array<double, 4>> identityQuats(size_t n) {
    return std::vector<std::array<double, 4>>(n, {1, 0, 0, 0});
  }

  /**
   * Writes a CK with one ID per supported writer, all in plain ticks:
   *   -85001 type 1, discrete epochs
   *   -85002 type 2, records with gaps between some of them
   *   -85003 type 3, two segments, one with a gap, overlapping each other
   *   -85005 type 5, with interpolation intervals and segment bounds that cut
   *                  into the data
   */
  void writeMixedCk(const std::string &path) {
    SpiceInt handle;
    ckopn_c(path.c_str(), "daf reader test", 0, &handle);

    {
      std::vector<double> ticks = {1000, 1010, 1020, 1020.5, 1500};
      auto q = identityQuats(ticks.size());
      ckw01_c(handle, ticks.front(), ticks.back(), -85001, "J2000", SPICEFALSE, "T1",
              ticks.size(), ticks.data(), (SpiceDouble (*)[4])q.data(), NULL);
    }

    {
      std::vector<double> starts = {2000, 2100, 2300, 2400};
      std::vector<double> stops  = {2100, 2200, 2350, 2500};
      auto q = identityQuats(starts.size());
      std::vector<std::array<double, 3>> av(starts.size(), {0, 0, 0});
      std::vector<double> rates(starts.size(), 1.0);
      ckw02_c(handle, starts.front(), stops.back(), -85002, "J2000", "T2", starts.size(),
              starts.data(), stops.data(), (SpiceDouble (*)[4])q.data(),
              (SpiceDouble (*)[3])av.data(), rates.data());
    }

    {
      // 250 records so the epoch directory is in play; intervals start at
      // 3000, 3400 and 3700 with no epochs between 3490 and 3700
      std::vector<double> ticks;
      for (int i = 0; i < 100; i++) ticks.push_back(3000 + i * 4);    // 3000..3396
      for (int i = 0; i < 25; i++) ticks.push_back(3400 + i * 3.75);  // 3400..3490
      for (int i = 0; i < 125; i++) ticks.push_back(3700 + i * 2);    // 3700..3948
      std::vector<double> starts = {3000, 3400, 3700};
      auto q = identityQuats(ticks.size());
      ckw03_c(handle, ticks.front(), ticks.back(), -85003, "J2000", SPICEFALSE, "T3A",
              ticks.size(), ticks.data(), (SpiceDouble (*)[4])q.data(), NULL,
              starts.size(), starts.data());

      // A second segment that overlaps the end of the first
      std::vector<double> ticks2 = {3900, 3950, 4000};
      std::vector<double> starts2 = {3900};
      auto q2 = identityQuats(ticks2.size());
      std::vector<std::array<double, 3>> av2(ticks2.size(), {0, 0, 0});
      ckw03_c(handle, ticks2.front(), ticks2.back(), -85003, "J2000", SPICETRUE, "T3B",
              ticks2.size(), ticks2.data(), (SpiceDouble (*)[4])q2.data(),
              (SpiceDouble (*)[3])av2.data(), starts2.size(), starts2.data());
    }

    {
      std::vector<double> ticks;
      for (int i = 0; i < 20; i++) ticks.push_back(5000 + i * 10);   // 5000..5190
      for (int i = 0; i < 20; i++) ticks.push_back(5500 + i * 10);   // 5500..5690
      std::vector<double> starts = {5000, 5500};
      auto q = identityQuats(ticks.size());
      // Segment bounds cut into both ends of the data
      ckw05_c(handle, C05TP1, 3, 5050, 5650, -85005, "J2000", SPICEFALSE, "T5",
              ticks.size(), ticks.data(), q.data(), 1.0, starts.size(), starts.data());
    }

    ckcls_c(handle);
    checkNaifErrors();
  }


  /**
   * Copies a little endian DAF to a big endian one by swapping every numeric
   * word, so the reader's byte order handling can be tested on a file CSPICE
   * wrote. Comment and name records are text and are copied as is.
   */
  void writeBigEndianCopy(const std::string &src, const std::string &dst) {
    std::ifstream in(src, std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_EQ(std::string(bytes.data() + 88, 8), "LTL-IEEE");

    auto swapAt = [&](size_t offset, size_t n) {
      std::reverse(bytes.begin() + offset, bytes.begin() + offset + n);
    };
    auto intAt = [&](size_t offset) {
      int32_t v;
      std::memcpy(&v, bytes.data() + offset, 4);
      return v;
    };
    auto doubleAt = [&](size_t offset) {
      double v;
      std::memcpy(&v, bytes.data() + offset, 8);
      return v;
    };

    const int nd = intAt(8);
    const int ni = intAt(12);
    const int fward = intAt(76);
    const int ss = nd + (ni + 1) / 2;
    const size_t nrec = bytes.size() / 1024;

    // Summary records, and the name record after each
    std::set<size_t> summaryRecs, nameRecs;
    for (int r = fward; r > 0; r = (int)doubleAt((size_t)(r - 1) * 1024)) {
      summaryRecs.insert(r);
      nameRecs.insert(r + 1);
    }

    for (size_t r = 1; r <= nrec; r++) {
      size_t base = (r - 1) * 1024;
      if (r == 1) {
        for (size_t off : {8, 12, 76, 80, 84}) swapAt(base + off, 4);
        std::memcpy(bytes.data() + base + 88, "BIG-IEEE", 8);
      }
      else if ((int)r < fward || nameRecs.count(r)) {
        continue;
      }
      else if (summaryRecs.count(r)) {
        int nsum = (int)doubleAt(base + 16);
        for (int w = 0; w < 3; w++) swapAt(base + w * 8, 8);
        for (int i = 0; i < nsum; i++) {
          size_t s = base + (3 + i * ss) * 8;
          for (int j = 0; j < nd; j++) swapAt(s + j * 8, 8);
          for (int j = 0; j < ni; j++) swapAt(s + nd * 8 + j * 4, 4);
        }
      }
      else {
        for (int w = 0; w < 128; w++) swapAt(base + w * 8, 8);
      }
    }

    std::ofstream out(dst, std::ios::binary);
    out.write(bytes.data(), bytes.size());
  }

}


TEST_F(TempTestingFiles, DafReaderCkTicksMatchCkcov) {
  std::string ckPath = (tempDir / "mixed.bc").string();
  writeMixedCk(ckPath);

  daf::File f(ckPath);
  EXPECT_EQ(f.kind(), "CK");
  EXPECT_EQ(f.nd(), 2);
  EXPECT_EQ(f.ni(), 6);
  EXPECT_EQ(f.summaries().size(), 5);

  std::map<int, daf::Window> cover = ckCoverageTicks(f);
  ASSERT_EQ(cover.size(), 4);
  for (int id : {-85001, -85002, -85003, -85005}) {
    SCOPED_TRACE(id);
    EXPECT_EQ(cover[id], cspiceCkCoverage(ckPath, id, "SCLK"));
  }

  // Spot check the windows themselves, not just agreement with CSPICE
  EXPECT_EQ(cover[-85001], daf::Window({{1000, 1000}, {1010, 1010}, {1020, 1020},
                                        {1020.5, 1020.5}, {1500, 1500}}));
  EXPECT_EQ(cover[-85002], daf::Window({{2000, 2200}, {2300, 2350}, {2400, 2500}}));
  EXPECT_EQ(cover[-85003], daf::Window({{3000, 3396}, {3400, 3490}, {3700, 4000}}));
  EXPECT_EQ(cover[-85005], daf::Window({{5050, 5190}, {5500, 5650}}));
}


TEST_F(TempTestingFiles, DafReaderBigEndian) {
  std::string ckPath = (tempDir / "mixed.bc").string();
  std::string bigPath = (tempDir / "mixed_big.bc").string();
  writeMixedCk(ckPath);
  writeBigEndianCopy(ckPath, bigPath);

  daf::File little(ckPath);
  daf::File big(bigPath);
  std::map<int, daf::Window> expected = ckCoverageTicks(little);
  EXPECT_EQ(ckCoverageTicks(big), expected);

  // CSPICE reads big endian files too, which checks the copy itself
  for (auto &[id, w] : expected) {
    SCOPED_TRACE(id);
    EXPECT_EQ(cspiceCkCoverage(bigPath, id, "SCLK"), w);
  }
}


TEST_F(TempTestingFiles, DafReaderNaifDafIdWord) {
  // Kernels from before the "DAF/CK" ID word say only "NAIF/DAF"
  std::string ckPath = (tempDir / "mixed.bc").string();
  std::string oldPath = (tempDir / "mixed_old.bc").string();
  writeMixedCk(ckPath);
  {
    std::ifstream in(ckPath, std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::memcpy(bytes.data(), "NAIF/DAF", 8);
    std::ofstream out(oldPath, std::ios::binary);
    out.write(bytes.data(), bytes.size());
  }

  daf::File f(oldPath);
  EXPECT_EQ(f.kind(), "");
  std::map<int, daf::Window> cover = ckCoverageTicks(f);
  EXPECT_EQ(cover, ckCoverageTicks(daf::File(ckPath)));
  for (auto &[id, w] : cover) {
    SCOPED_TRACE(id);
    EXPECT_EQ(cspiceCkCoverage(oldPath, id, "SCLK"), w);
  }
}


TEST_F(TempTestingFiles, DafReaderLongType3MatchesCkcov) {
  // Enough records and intervals of uneven lengths that finding each
  // interval's last epoch reads ahead through the file and wraps around the
  // record cache. The epochs jump every 1000 records.
  std::vector<double> ticks;
  double t = 10000;
  for (int i = 0; i < 20000; i++) {
    ticks.push_back(t);
    t += (i % 1000 == 999) ? 500 : 3;
  }
  std::vector<double> starts = {ticks[0]};
  for (size_t i = 1, k = 0; ; k++) {
    i += 1 + (k * 37) % 97;
    if (i >= ticks.size()) break;
    starts.push_back(ticks[i]);
  }
  ASSERT_GT(starts.size(), 300);

  std::string ckPath = (tempDir / "long3.bc").string();
  auto q = identityQuats(ticks.size());
  SpiceInt handle;
  ckopn_c(ckPath.c_str(), "daf reader test", 0, &handle);
  ckw03_c(handle, ticks.front(), ticks.back(), -85003, "J2000", SPICEFALSE, "LONG",
          ticks.size(), ticks.data(), (SpiceDouble (*)[4])q.data(), NULL,
          starts.size(), starts.data());
  ckcls_c(handle);
  checkNaifErrors();

  daf::File f(ckPath);
  daf::Window cover = ckCoverageTicks(f)[-85003];
  EXPECT_EQ(cover, cspiceCkCoverage(ckPath, -85003, "SCLK"));
  EXPECT_GT(cover.size(), 300);
}


TEST_F(TempTestingFiles, DafReaderSpkMatchesSpkcov) {
  std::string spkPath = (tempDir / "coverage.bsp").string();

  struct Seg { int body; int center; double start; double stop; };
  std::vector<Seg> segs = {{-85, 301, 400, 500}, {-85, 301, 100, 200},
                           {-85, 301, 150, 250}, {-85, 301, 250, 300},
                           {301, 3, 100, 500}};

  SpiceInt handle;
  spkopn_c(spkPath.c_str(), "daf reader test", 0, &handle);
  for (auto &seg : segs) {
    SpiceDouble states[2][6] = {{1, 2, 3, 0.1, 0.2, 0.3}, {4, 5, 6, 0.4, 0.5, 0.6}};
    SpiceDouble epochs[2] = {seg.start, seg.stop};
    spkw13_c(handle, seg.body, seg.center, "J2000", seg.start, seg.stop, "SEG", 1, 2, states, epochs);
  }
  spkcls_c(handle);
  checkNaifErrors();

  daf::File f(spkPath);
  EXPECT_EQ(f.kind(), "SPK");
  std::map<int, daf::Window> cover = spkCoverage(f);
  ASSERT_EQ(cover.size(), 2);
  EXPECT_EQ(cover[-85], cspiceSpkCoverage(spkPath, -85));
  EXPECT_EQ(cover[301], cspiceSpkCoverage(spkPath, 301));

  // Overlapping and touching segments merge
  EXPECT_EQ(cover[-85], daf::Window({{100, 300}, {400, 500}}));
}


TEST_F(LroKernelSet, DafReaderCkEtMatchesCkcov) {
  Kernel lsk(lskPath);
  Kernel sclk(sclkPath);

  std::string ckPath = (root / "ck" / "daf_reader_et.bc").string();
  std::vector<double> ets = {110000000, 111000000, 112000000, 118000000, 119000000, 120000000};
  std::vector<double> ticks(ets.size());
  for (size_t i = 0; i < ets.size(); i++) {
    sce2c_c(-85, ets[i], &ticks[i]);
  }
  auto q = identityQuats(ticks.size());
  SpiceDouble starts[2] = {ticks[0], ticks[3]};

  SpiceInt handle;
  ckopn_c(ckPath.c_str(), "daf reader et", 0, &handle);
  ckw03_c(handle, ticks.front(), ticks.back(), -85000, "J2000", SPICEFALSE, "ET",
          ticks.size(), ticks.data(), (SpiceDouble (*)[4])q.data(), NULL, 2, starts);
  ckcls_c(handle);
  checkNaifErrors();

  daf::File f(ckPath);
  // Converted to ET after merging, as convertDafCoverage does
  std::map<int, daf::Window> cover = ckCoverageTicks(f);
  for (auto &[id, w] : cover) {
    SpiceInt clock;
    ckmeta_c(id, "SCLK", &clock);
    for (auto &iv : w) {
      sct2e_c(clock, iv.first, &iv.first);
      sct2e_c(clock, iv.second, &iv.second);
    }
    checkNaifErrors();
    w = daf::merge(std::move(w));
  }

  ASSERT_EQ(cover.size(), 1);
  EXPECT_EQ(cover[-85000], cspiceCkCoverage(ckPath, -85000, "TDB"));
  ASSERT_EQ(cover[-85000].size(), 2);
  EXPECT_NEAR(cover[-85000][0].first, 110000000, 1e-3);
  EXPECT_NEAR(cover[-85000][0].second, 112000000, 1e-3);
  EXPECT_NEAR(cover[-85000][1].first, 118000000, 1e-3);
  EXPECT_NEAR(cover[-85000][1].second, 120000000, 1e-3);
}


TEST_F(TempTestingFiles, DafReaderRejectsNonDaf) {
  std::string textPath = (tempDir / "not_a_daf.bc").string();
  {
    std::ofstream out(textPath);
    out << std::string(2048, 'x');
  }
  EXPECT_THROW(daf::File f(textPath), std::runtime_error);
  EXPECT_THROW(daf::File f((tempDir / "missing.bc").string()), std::runtime_error);
}


TEST(DafReader, MergeMatchesWninsd) {
  daf::Window w = {{5, 6}, {1, 2}, {2, 3}, {8, 8}, {7, 9}, {10, 10}, {10, 10}};
  EXPECT_EQ(daf::merge(w), daf::Window({{1, 3}, {5, 6}, {7, 9}, {10, 10}}));
  EXPECT_TRUE(daf::merge({}).empty());
}
