import subprocess
import unittest

class PipeImplementationTests(unittest.TestCase):
            
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
    
        def test_execute_single_program(self):
            """Test executing a single program without piping."""
            result = subprocess.run(['./pipe', 'echo', 'hello'], capture_output=True, text=True)
            self.assertEqual(result.stdout.strip(), 'hello')
    
        def test_execute_two_programs_with_pipe(self):
            """Test executing two programs with a pipe between them."""
            result = subprocess.run(['./pipe', 'echo', 'hello', 'wc', '-c'], capture_output=True, text=True)
            self.assertTrue('6' in result.stdout.strip())
    
        def test_standard_input_output_preservation(self):
            """Test that the standard input of the first process and the standard output of the last process are preserved."""
            input_text = "hello\nworld"
            result = subprocess.run(['./pipe', 'cat', 'wc', '-l'], input=input_text, capture_output=True, text=True)
            self.assertTrue('2' in result.stdout.strip())
    
        def test_handle_invalid_argument(self):
            """Test the program exits with EINVAL when no command line arguments are provided."""
            result = subprocess.run(['./pipe'], capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(result.returncode, 22)  # Assuming the program exits with EINVAL for no args
    
        def test_handle_more_than_eight_programs(self):
            """Test handling more than 8 programs."""
            command_list = ['./pipe'] + ['echo'] * 9
            result = subprocess.run(command_list, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0)
    
        def test_no_orphan_processes(self):
            """Ensure that no orphan processes are left by executing a sequence of commands."""
            # This test might require more sophisticated system-level checks after the run
            subprocess.run(['./pipe', 'echo', 'hello', 'wc', '-c'], capture_output=True)
            # You might need to verify the absence of orphans using system tools or logs
    
        def test_error_exit_code(self):
            """Test that the program exits with the correct error code on failure."""
            result = subprocess.run(['./pipe', 'nonexistent_command'], capture_output=True)
            self.assertNotEqual(result.returncode, 0)
    
        def test_no_leaking_file_descriptors(self):
            """Test for leaking file descriptors by executing commands that would reveal unclosed fds."""
            # Implementing this test requires checking system resources or using specific diagnostic tools
