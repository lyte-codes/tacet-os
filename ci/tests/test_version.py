import subprocess
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import version  # noqa: E402


class TagsTest(unittest.TestCase):
    def test_stable_release(self):
        self.assertEqual(
            version.tags("44.3.1", "stable", "20260913"),
            ["44.3.1", "44.3", "44", "stable", "44.3.1-20260913"],
        )

    def test_prerelease_only_exact_and_channel(self):
        self.assertEqual(
            version.tags("44.3.0-rc1", "testing", "20260913"),
            ["44.3.0-rc1", "testing", "44.3.0-rc1-20260913"],
        )

    def test_prerelease_refuses_stable(self):
        with self.assertRaises(ValueError):
            version.tags("44.3.0-rc1", "stable", "")

    def test_no_date(self):
        self.assertEqual(version.tags("44.0.0", "testing", ""), ["44.0.0", "44.0", "44", "testing"])


class BumpTest(unittest.TestCase):
    def test_patch(self):
        self.assertEqual(version.bump("patch", "44.3.1"), "44.3.2")

    def test_major_resets_patch(self):
        self.assertEqual(version.bump("major", "44.3.1"), "44.4.0")

    def test_fedora_keeps_major_resets_patch(self):
        self.assertEqual(version.bump("fedora", "43.2.5"), "44.2.0")

    def test_rejects_garbage(self):
        for bad in ["44.3", "v44.3.1", "44.3.1.2", "tacet-44.3.1"]:
            with self.assertRaises(ValueError):
                version.parse(bad)


class CliTest(unittest.TestCase):
    def test_check_tag(self):
        vf = Path(__file__).resolve().parent / "VERSION.tmp"
        vf.write_text("44.3.1\n")
        try:
            script = Path(__file__).resolve().parent.parent / "version.py"
            ok = subprocess.run([sys.executable, script, "check-tag", "v44.3.1", str(vf)])
            bad = subprocess.run([sys.executable, script, "check-tag", "v44.3.2", str(vf)], capture_output=True)
            self.assertEqual(ok.returncode, 0)
            self.assertEqual(bad.returncode, 1)
        finally:
            vf.unlink()


if __name__ == "__main__":
    unittest.main()
