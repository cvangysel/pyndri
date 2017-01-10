class Pyndri < Formula
  desc ""
  homepage ""
  url "https://github.com/MatsWillemsen/pyndri/archive/1.0.zip"
  sha256 "bceee4a5884a550373080eed9d55b88def4b6f34dfe0cd763320683c1697b9bb"

  head "https://github.com/MatsWillemsen/pyndri.git", :branch => "master"
  depends_on :python3
  depends_on "bpiwowar/misc/indri" => :build
  depends_on "bpiwowar/misc/trec_eval" => :build

  def install
     system "python3", *Language::Python.setup_install_args(prefix)
  end

  test do
   system "false"
  end
end
