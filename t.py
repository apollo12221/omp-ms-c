#!/usr/bin/env python
"""Main module responsible for testing the software."""

import sys
import os
import unittest
import json
import time

sys.path.append(os.path.dirname(os.getcwd()))

from components import reader
from components import writer
from components import topology_parser as top_par

from components.attack_graph_parser import breadth_first_search
from components.attack_graph_parser import generate_attack_graph

def scalability_test_helper(example_folder):
    """Main function that tests the scalability for different examples."""

    number_runs_per_test = 1

    duration_topology = 0
    duration_bdf = 0
    duration_vuls_preprocessing = 0
    duration_total_time = 0

    for i in range(1, number_runs_per_test+1):

        print("\n\nIteration: "+str(i))
        total_time_start = time.time()

        # Preparing the data for testing
        parent_path = os.path.dirname(os.getcwd())

        # Opening the configuration file.
        config = reader.read_config_file(old_root_path=parent_path)
        topology, duration_topology_new = top_par.parse_topology(example_folder,
                                                                 os.getcwd())

        #print(topology)
        duration_topology += duration_topology_new

        vuls_orig = reader.read_vulnerabilities(example_folder,
                                                ["example_samba",
                                                 "example_phpmailer"])

        vulnerabilities = {}
        for container_orig in vuls_orig.keys():
            for container_topology in topology.keys():
                if container_orig in container_topology:
                    vulnerabilities[container_topology] = vuls_orig[container_orig]

        att_vec_path = os.path.join(parent_path, config["attack-vector-folder-path"])

        # Returns a tuple of the form:
        # (attack_graph_nodes, attack_graph_edges, duration_bdf, duration_vul_preprocessing)
        attack_graph_tuple = generate_attack_graph(att_vec_path,
                                                   config["preconditions-rules"],
                                                   config["postconditions-rules"],
                                                   topology,
                                                   vulnerabilities,
                                                   example_folder)

        # Unpacking the variables
        duration_bdf += attack_graph_tuple[2]
        duration_vuls_preprocessing += attack_graph_tuple[3]

        no_topology_nodes = len(topology.keys())
        no_topology_edges = 0
        for container in topology.keys():
            no_topology_edges += len(topology[container])

        # We divide them by two because each edge is counted twice.
        no_topology_edges = int(no_topology_edges / 2)
        no_attack_graph_nodes = len(attack_graph_tuple[0])
        no_attack_graph_edges = len(attack_graph_tuple[1])

        duration_total_time += (time.time() - total_time_start)

    # Calculate the averages of the times
    duration_topology = duration_topology/number_runs_per_test
    duration_vuls_preprocessing = duration_vuls_preprocessing/number_runs_per_test
    duration_bdf = duration_bdf/number_runs_per_test
    duration_total_time = duration_topology + duration_vuls_preprocessing + duration_bdf

    # Printing time summary of the attack graph generation.
    writer.print_summary(config["mode"],
                         config["generate_graphs"],
                         no_topology_nodes=no_topology_nodes,
                         no_topology_edges=no_topology_edges,
                         no_attack_graph_nodes=no_attack_graph_nodes,
                         no_attack_graph_edges=no_attack_graph_edges,
                         duration_topology=duration_topology,
                         duration_vuls_preprocessing=duration_vuls_preprocessing,
                         duration_bdf=duration_bdf,
                         duration_total_time=duration_total_time)

    print("Total time elapsed: "+str(duration_total_time)+"\n\n\n")

class MyTest(unittest.TestCase):

    def test_scalability_1(self):
        """Doing scalability testing of samba and phpmailer example. It has
        1 phpmailer container and 1 samba container."""

        print("Test: Scalability test of samba and phpmailer example...")

        # Preparing the data for testing
        example_folder = os.path.join(os.getcwd(), "1_example")
        #scalability_test_helper(example_folder)

    def test_scalability_5(self):
        """Doing scalability testing of samba and phpmailer example. It has
        1 phpmailer container and 5 samba containers."""

        print("Test: Scalability test of 5 samba and phpmailer example...")

        # Preparing the data for testing
        example_folder = os.path.join(os.getcwd(), "5_example")
        #scalability_test_helper(example_folder)


    def test_scalability_20(self):
        """Doing scalability testing of samba and phpmailer example. It has
        1 phpmailer container and 20 samba containers."""

        print("Test: Scalability test of 20 samba and phpmailer example...")

        # Preparing the data for testing
        example_folder = os.path.join(os.getcwd(), "20_example")
        #scalability_test_helper(example_folder)

    def test_scalability_50(self):
        """Doing scalability testing of samba and phpmailer example. It has
        1 phpmailer container and 50 samba containers."""

        print("Test: Scalability test of 50 samba and phpmailer example...")

        # Preparing the data for testing
        example_folder = os.path.join(os.getcwd(), "50_example")
        #scalability_test_helper(example_folder)

    def test_scalability_100(self):
        """Doing scalability testing of samba and phpmailer example. It has
        1 phpmailer container and 100 samba containers."""

        print("Test: Scalability test of 100 samba and phpmailer example...")

        # Preparing the data for testing
        example_folder = os.path.join(os.getcwd(), "100_example")
        #scalability_test_helper(example_folder)

    def test_scalability_500(self):
        """Doing scalability testing of samba and phpmailer example. It has
        1 phpmailer container and 500 samba containers."""

        print("Test: Scalability test of 500 samba and phpmailer example...")

        # Preparing the data for testing
        example_folder = os.path.join(os.getcwd(), "500_example")
        #scalability_test_helper(example_folder)

    def test_scalability_1000(self):
        """Doing scalability testing of samba and phpmailer example. It has
        1 phpmailer container and 1000 samba containers."""

        print("Test: Scalability test of 1000 samba and phpmailer example...")

        # Preparing the data for testing
        example_folder = os.path.join(os.getcwd(), "1000_example")
        scalability_test_helper(example_folder)

if __name__ == "__main__":
    print("Testing the attack graph generator...")

    unittest.main()
