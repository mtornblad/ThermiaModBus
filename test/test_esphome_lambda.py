"""Compile and exercise the actual YAML lambda, independent of ESP-IDF tooling."""
from pathlib import Path
import subprocess
import tempfile
import yaml

root = Path(__file__).resolve().parents[1]
config = yaml.safe_load((root / 'esphome/packages/raw-register.yaml').read_text())
body = config['sensor'][0]['lambda']
program = '''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <span>
struct Item { uint8_t offset; };
float convert(float x, std::span<const uint8_t> data, Item* item) {
''' + body + '''
}
int main() {
  Item first{0};
  const uint8_t fresh[] = {0, 0, 0, 1, 0, 0};
  const uint8_t unseen[] = {0, 0, 0, 0, 0xff, 0xff};
  const uint8_t stale[] = {0, 42, 0, 2, 0, 120};
  assert(convert(0, fresh, &first) == 0);  // valid zero is not hidden
  assert(std::isnan(convert(0, unseen, &first)));
  assert(std::isnan(convert(42, stale, &first)));
  assert(std::isnan(convert(42, std::span(fresh, 3), &first)));
  Item second{6};
  const uint8_t combined[] = {0, 42, 0, 2, 0, 120, 0xff, 0x9c, 0, 1, 0, 0};
  assert(convert(65436, combined, &second) == 65436); // same-response offset
}
'''
with tempfile.TemporaryDirectory() as directory:
    cpp = Path(directory) / 'lambda.cpp'
    executable = Path(directory) / 'lambda'
    cpp.write_text(program)
    subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-pedantic', str(cpp), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print('ESPHome YAML lambda: valid zero, unseen/stale, short reply and offset tests passed')
