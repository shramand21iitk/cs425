import dns.message
import dns.query
import dns.rdatatype
import dns.resolver
import time
import sys #Allows access to command-line arguments. It is used to read the mode and domain name from the command line when the script is executed.
import re #Provides support for regular expressions. It is used to check if the domain name is in the correct format (contains at least one period).
import idna #Provides support for Internationalized Domain Names (IDN). It is used to convert Unicode domain names to ASCII Punycode format for DNS resolution.

# Root DNS servers used to start the iterative resolution process. The root servers will be the first set of servers to be queried in iterative DNS lookup.
# The IP addresses of the root servers are hardcoded in this dictionary. The keys are the IP addresses of the root servers, and the values are the names of the root servers.
ROOT_SERVERS = {
    "198.41.0.4": "Root (a.root-servers.net)",
    "199.9.14.201": "Root (b.root-servers.net)",
    "192.33.4.12": "Root (c.root-servers.net)",
    "199.7.91.13": "Root (d.root-servers.net)",
    "192.203.230.10": "Root (e.root-servers.net)"
}

TIMEOUT = 3  # Timeout in seconds for each DNS query attempt
MAX_RETRIES = 3  # Number of retries before failing

def send_dns_query(server, domain):
    # server: The IP address of the DNS server to query
    # domain: The domain name for which we want to resolve an IP address
    """ 
    Sends a DNS query to the given server for an A record of the specified domain.
    Returns the response if successful, otherwise returns None.
    """
    '''
    New Implements:
    - Retry with exponential backoff (1s → 2s → 4s)
    - Fallback to TCP if UDP fails
    '''
    # Construct the DNS query message using dnspython library
    query = dns.message.make_query(domain, dns.rdatatype.A)  #Constructs a DNS query packet requesting an A record (IPv4 address) for 'domain'
    for attempt in range(MAX_RETRIES):
        try:
            wait_time = 2**attempt #Exponential backoff: [1, 2, 4] seconds
            # TODO: Send the query using UDP 
            # Note that above TODO can be just a return statement with the UDP query!
            response = dns.query.udp(query, server, timeout=TIMEOUT) #Sends the query to the given server over UDP and waits for a response. If the response is not received within the TIMEOUT period, a Timeout exception is raised.
            return response # Return response if successful
        except dns.exception.Timeout:
            print(f"[ERROR] Timeout while querying {server} for {domain}") # Print error message if timeout occurs
        except dns.query.BadResponse:
            print(f"[ERROR] Bad response from {server} while querying {domain}") # Print error message if an invalid or malformed response is received
            break  # Stop retrying on bad response
        except Exception as e:
            print(f"[ERROR] Unexpected error querying {server}: {e}") # Print error message for any other unexpected errors that might occur during the DNS query
            break  # Stop retrying on bad response

    # If UDP fails, fallback to TCP
    print(f"[INFO] Falling back to TCP for {server}...")
    try:
        response = dns.query.tcp(query, server, timeout=TIMEOUT * 2)  # Similar to UDP use TCP which is usually slower.
        return response
    except dns.exception.Timeout: #
        print(f"[ERROR] TCP query to {server} timed out.") 
    except Exception as e:
        print(f"[ERROR] Unexpected TCP error querying {server}: {e}")

    return None  # Return None if any error occurred, indicating that the query was unsuccessful

def extract_next_nameservers(response):
    """ 
    Extracts nameserver (NS) records from the authority section of the response.
    Then, resolves those NS names to IP addresses.
    Returns a list of IPs of the next authoritative nameservers.
    """
    ns_ips = []  # List to store the IP addresses of the next nameservers
    ns_names = []  # List to store the hostnames (domain names) of the next nameservers

    if not response.authority: #Checks if the response has an "authority section" which contains nameservers that are authoritative for the queried domain
        print("[ERROR] No authority section found in the response.")
        return ns_ips  # Return empty list if no next nameservers could be extracted
    
    # Loop through each record set (rrset) in the authority section to extract NS records
    for rrset in response.authority: #Iterates over each record set (rrset) in the authority section. An rrset (Resource Record Set) can contain multiple DNS records of the same type (e.g., multiple NS records).
        if rrset.rdtype == dns.rdatatype.NS: #Checks if the record type is NS (Nameserver Record) i.e. the response contains a reference to another nameserver that is authoritative for the query domain
            for rr in rrset: #Iterates over each record (rr) in the NS record set. Each rr contains a hostname (domain name) of a nameserver.
                ns_name = rr.to_text()
                ns_names.append(ns_name)  # Extract the nameserver hostname from the record and adds it to ns_names.
                print(f"Extracted NS hostname: {ns_name}")
    # After extracting nameserver hostnames, we need to resolve them to IP addresses
    # TODO: Resolve the extracted NS hostnames to IP addresses
    # To TODO, you would have to write a similar loop as above
    for ns_name in ns_names: #Iterates through each extracted nameserver hostname to resolve its IP
        try:
            answer = dns.resolver.resolve(ns_name, "A") #Attempts to resolve the nameserver’s hostname to an IPv4 address (A record).Queries the default system DNS resolver for the IP address of ns_name
            for rdata in answer: #Loops through the response and extracts IP addresses.
                ip_address = rdata.to_text() #Converts the IP address to a string
                ns_ips.append(rdata.to_text())  # Converts the IP address to a string and adds it to ns_ips
                print(f"Resolved {ns_name} to {ip_address}")
        except dns.resolver.NXDOMAIN: #The nameserver domain does not exist
            print(f"[WARNING] Nameserver {ns_name} does not exist.")
        except dns.resolver.NoAnswer: #The nameserver domain exists but has no A record (IPv4 address).
            print(f"[WARNING] No A record found for nameserver {ns_name}.")
        except dns.exception.Timeout: #The query for resolving the nameserver took too long.
            print(f"[ERROR] Timeout while resolving NS {ns_name}.")
        except Exception as e: #Handles any other unexpected errors and logs them.
            print(f"[ERROR] Failed to resolve NS {ns_name}: {e}")
    
    return ns_ips  # Return list of resolved nameserver IPs. This list will be used by iterative_dns_lookup() to continue querying the next nameservers

def iterative_dns_lookup(domain):
    """ 
    Performs an iterative DNS resolution starting from root servers.
    It queries root servers, then TLD servers, then authoritative servers,
    following the hierarchy until an answer is found or resolution fails.
    """
    print(f"[Iterative DNS Lookup] Resolving {domain}")

    next_ns_list = list(ROOT_SERVERS.keys())  # Initializes next_ns_list with the IP addresses of root DNS servers (from ROOT_SERVERS) which are the first set of servers to be queried.
    stage = "ROOT"  # Initializes stage to "ROOT", indicating that the first set of queries will be sent to the root DNS servers. As the resolution progresses, this value will change to "TLD" (Top-Level Domain servers) and then "AUTH" (Authoritative servers).

    while next_ns_list: #Loops as long as there are nameservers in next_ns_list. Each iteration queries a nameserver to continue the resolution process.
        ns_ip = next_ns_list[0]  # Pick the first available nameserver to query
        response = send_dns_query(ns_ip, domain) #Calls send_dns_query(ns_ip, domain) to send a DNS query for an A record (IPv4 address) to the current nameserver.
        
        if response: #checks if response is not NONE i.e if a valid DNS response is received
            print(f"[DEBUG] Querying {stage} server ({ns_ip}) - SUCCESS")
            
            if response.answer: # If an answer is found, print and return i.e. if we have successfully resolved the domain to an IP address
                print(f"[SUCCESS] {domain} -> {response.answer[0][0]}")
                return
            #If the response did not contain an answer, it means the queried nameserver does not have the final answer. Instead, it returns a list of authoritative nameservers in the authority section.
            # If no answer contained in the response, extract the next set of nameservers
            next_ns_list = extract_next_nameservers(response) #Extracts nameserver hostnames, resolves them to IP addresses, and Returns the list of IPs to be queried next by storing them in next_ns_list.
            # TODO: Move to the next resolution stage, i.e., it is either TLD, ROOT, or AUTH
            if stage == "ROOT": #After querying a root server, the next step is the TLD nameservers. Therefore, the stage is updated to "TLD".
                stage = "TLD"
            elif stage == "TLD": #After querying a TLD server, the next step is the authoritative nameservers. Therefore, the stage is updated to "AUTH".
                stage = "AUTH"

        else:
            print(f"[ERROR] Query failed for {stage} {ns_ip}")
            return  # Stop resolution if a query fails
    
    print("[ERROR] Resolution failed.")  # Final failure message if no nameservers respond

def recursive_dns_lookup(domain):
    """ 
    Performs recursive DNS resolution using the system's default resolver.
    This approach relies on a resolver (like Google DNS or a local ISP resolver) to fetch the result recursively.
    Directly retrieves the IP address without iterative queries.
    """
    print(f"[Recursive DNS Lookup] Resolving {domain}")
    try:
        # Get authoritative name servers
        ns_records = dns.resolver.resolve(domain, 'NS') 
        for ns in ns_records: #Iterates over each NS record retrieved from the query response.
            print(f"[SUCCESS] {domain} -> {ns}")
        # TODO: Perform recursive resolution using the system's DNS resolver
        # Notice that the next line is looping through, therefore you should have something like answer = ??
        answer = dns.resolver.resolve(domain, "A")  # Send a query for an A record (IPv4 address) to the system's configured DNS resolver.
        #The resolver handles all recursion automatically, meaning it follows the DNS hierarchy (Root → TLD → Authoritative) without the script needing to do so manually.
        #If successful, answer will contain a list of IP addresses corresponding to the domain.

        for rdata in answer: #Loops through each response entry (rdata) in the DNS answer since a domain can have multiple A records, meaning it may resolve to multiple IP addresses.
            print(f"[SUCCESS] {domain} -> {rdata}")

    except dns.resolver.NXDOMAIN: #The requested domain name does not exist in the DNS system.
        print(f"[ERROR] The domain {domain} does not exist.")
    except dns.resolver.NoAnswer: #The domain exists but has no A record (IPv4 address).
        print(f"[ERROR] No answer for {domain}. The domain exists but has no A record.")
    except dns.resolver.Timeout: #The query took too long to resolve.
        print(f"[ERROR] Timeout while resolving {domain}.")
    except Exception as e: #Handles any other unexpected errors and logs them.
        print(f"[ERROR] Recursive lookup failed: {e}")

# Helper functions for domain validation and normalization
def is_valid_domain(domain):
    '''
    Allows single-character domains (x.org)
    Prevents invalid TLDs (.xyz123 is blocked)
    Prevents double dots (example..com)
    Restricts labels to 1-63 characters
    '''
    pattern = r"^(?!-)([a-zA-Z0-9\-]{1,63}\.)+[a-zA-Z0-9]{2,63}\.?$" #Regular expression pattern to validate domain names. It checks if the domain name consists of alphanumeric characters, hyphens, and periods, and follows the correct format.
    return bool(re.match(pattern, domain)) #Checks if the domain name matches the regular expression pattern. If it does, the domain is valid; otherwise, it is invalid.
# Allows us to fully support international domains
def normalize_domain(domain): 
    try:
        return idna.encode(domain).decode()  # Converts the Unicode domain name to ASCII Punycode format using the idna library. This is done to ensure that the domain name is in a format that can be resolved by the DNS system.
    except idna.IDNAError:
        return None  # Invalid domain

if __name__ == "__main__": #Ensures that the code only runs when the script is executed directly
    '''
    sys.argv[0] is the script name (dns_server.py)
    sys.argv[1] is the mode (iterative or recursive)
    sys.argv[2] is the domain name to resolve
    '''
    if len(sys.argv) != 3 or sys.argv[1] not in {"iterative", "recursive"}: #Checks if the correct number of arguments is provided and the mode is either iterative or recursive.
        print("Usage: python3 dns_server.py <iterative|recursive> <domain>")
        sys.exit(1) # Exit the script if the arguments are invalid

    mode = sys.argv[1]  # Get mode (iterative or recursive)
    domain = sys.argv[2]  # Get domain to resolve\
    domain = normalize_domain(domain) #Calls the normalize_domain(domain) function to convert the Unicode domain name to ASCII Punycode format.

    if mode not in {"iterative", "recursive"}: #Checks if the mode is either iterative or recursive.
        print("Error: First argument must be 'iterative' or 'recursive'.")
        sys.exit(1) # Exit the script if the mode is invalid

    if domain and is_valid_domain(domain): #Checks if the domain name is not empty and is valid.
        pass
    else:
        print("Invalid domain")

    if "." not in domain: #Checks if the domain name is in the correct format (contains at least one period).
        print("Error: Invalid domain format.")
        sys.exit(1) # Exit the script if the domain name is invalid
        
    start_time = time.time()  # Record start time to calculate execution time
    
    # Execute the selected DNS resolution mode
    if mode == "iterative": 
        iterative_dns_lookup(domain) #Calls the iterative_dns_lookup(domain) function to perform iterative DNS resolution.
    else:
        recursive_dns_lookup(domain) #Calls the recursive_dns_lookup(domain) function to perform recursive DNS resolution.
    
    print(f"Time taken: {time.time() - start_time:.3f} seconds")  # Print execution time after resolution is complete.