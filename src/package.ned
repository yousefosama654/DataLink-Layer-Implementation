package datalink;

import ned.IdealChannel;


@license(LGPL);
//
//
network Network
{
    submodules:
        node0: Node {
            @display("p=78,52");
        }
        node1: Node {
            @display("p=177,52");
        }
        coordinator: Coordinator {
            @display("p=125,161");
        }
    connections:
        node0.out --> IdealChannel --> node1.in;
        node1.out --> IdealChannel --> node0.in;
        coordinator.out0 --> IdealChannel --> node0.coordinator_gate;
        coordinator.out1 --> IdealChannel --> node1.coordinator_gate;
}

