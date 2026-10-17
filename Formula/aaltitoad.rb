# aaltitoad - a verification engine for tick tock automata models
#   Copyright (C) 2026 Asger Gitz-Johansen
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# The Homebrew formula of aaltitoad. The package workflow tests it (installing it from a local tap, built from the
# checkout), and attaches it to the release of a version tag with the checksum of the tag's source archive.
class Aaltitoad < Formula
  desc "Verification engine and simulator for tick tock automata"
  homepage "https://github.com/sillydan1/aaltitoad"
  url "https://github.com/sillydan1/aaltitoad/archive/refs/tags/v1.2.1.tar.gz"
  sha256 "0000000000000000000000000000000000000000000000000000000000000000"
  license "GPL-3.0-or-later"
  head "https://github.com/sillydan1/aaltitoad.git", branch: "master"

  depends_on "bison" => :build
  depends_on "cmake" => :build
  depends_on "flex" => :build
  depends_on "z3"

  def install
    # NOTE: the dependencies of aaltitoad are downloaded by CPM while it is configured
    system "cmake", "-S", ".", "-B", "build", "-DCMAKE_POLICY_VERSION_MINIMUM=3.5", *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    assert_match "aaltitoad v", shell_output("#{bin}/verifier --version")
    assert_match "hawk_parser", shell_output("#{bin}/verifier -L -f . -Q 'E F true'")
  end
end
