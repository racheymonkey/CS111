import pathlib
import re
import subprocess
import unittest
import tempfile

class TestLab1(unittest.TestCase):

    def _make():
        result = subprocess.run(['make'], capture_output=True, text=True)
        return result

    def _make_clean():
        result = subprocess.run(['make', 'clean'],
                                capture_output=True, text=True)
        return result

    @classmethod
    def setUpClass(cls):
        cls.make = cls._make().returncode == 0

    @classmethod
    def tearDownClass(cls):
        cls._make_clean()

    def test_complex_scenario(self):
        self.assertTrue(self.make, msg='make failed')
    
        # Generate a large list of files in a temporary directory
        with tempfile.TemporaryDirectory() as temp_dir:
            file_count = 100
            for i in range(file_count):
                pathlib.Path(f"{temp_dir}/file_{i}.txt").touch()
    
            # Run a complex series of commands that involves listing files, 
            # grepping for a specific pattern, sorting the output, and counting the results
            expected_command = f"ls {temp_dir} | grep 'file_' | sort | uniq | wc -l"
            expected_result = subprocess.run(expected_command, capture_output=True, shell=True, text=True).stdout.strip()
    
            pipe_command = ['./pipe', 'ls', temp_dir, f"grep 'file_'", 'sort', 'uniq', 'wc', '-l']
            pipe_result = subprocess.run(pipe_command, capture_output=True, text=True)
    
            self.assertEqual(pipe_result.returncode, 0, msg='Complex scenario failed, expected return code 0.')
            self.assertEqual(pipe_result.stdout.strip(), expected_result, 
                             msg=f"Expected {expected_result} files, but got {pipe_result.stdout.strip()} instead.")
        
        # Test with a mix of valid and invalid commands to ensure error handling
        mixed_result = subprocess.run(['./pipe', 'ls', 'invalid_command', 'wc'], capture_output=True, stderr=subprocess.PIPE)
        self.assertNotEqual(mixed_result.returncode, 0, msg='Expected nonzero return code with invalid command.')
        self.assertTrue(mixed_result.stderr, msg='Expected error message with invalid command.')
    
        self.assertTrue(self._make_clean, msg='make clean failed')
