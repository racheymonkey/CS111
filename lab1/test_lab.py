import subprocess
import unittest

class ExtendedTestLab1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Setup common requirement for all tests, like compiling if necessary
        pass

    @classmethod
    def tearDownClass(cls):
        # Cleanup after all tests, like removing compiled binary
        pass

    def test_single_command(self):
        """Test executing a single program."""
        result = subprocess.run(['./pipe', 'echo', 'Hello'], capture_output=True, text=True)
        self.assertEqual(result.stdout.strip(), 'Hello')

    def test_multiple_commands_over_limit(self):
        """Test executing more than 8 programs."""
        commands = ['./pipe'] + ['echo'] * 9  # Adjust based on actual limit
        result = subprocess.run(commands, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0, "Should handle more than 8 commands gracefully.")

    def test_invalid_command(self):
        """Test error handling for invalid command line arguments."""
        result = subprocess.run(['./pipe', 'nonexistent_command'], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)

    def test_stderr_consistency(self):
        """Verify that stderr from child processes is the same as parent's."""
        result = subprocess.run(['./pipe', 'ls', 'nonexistent_folder'], capture_output=True, text=True)
        self.assertTrue("No such file or directory" in result.stderr)

    def test_proper_fd_closure(self):
        """Check for proper closure of file descriptors."""
        # This test case might require inspection of open file descriptors or specific error messages indicating unclosed fds.

    def test_error_exit_code(self):
        """Verify that the program exits with the proper errno."""
        # This may require specific conditions that trigger known errors to verify the correct exit code is used.

    def test_no_orphans_varied_commands(self):
        """Ensuring no orphan processes are created with varied numbers of commands."""
        # This test can be similar to the existing 'test_no_orphans' but with different combinations of commands.

    def test_nonexistent_program(self):
        """Testing with a non-existent program to verify EINVAL is returned."""
        result = subprocess.run(['./pipe'], capture_output=True)
        self.assertEqual(result.returncode, 22, "Expected EINVAL for no arguments.")

# Additional utility functions or setup/teardown methods as needed
