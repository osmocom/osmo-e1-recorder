#!/usr/bin/env python

app_configs = {
    "osmo-e1-recorder": ["doc/examples/osmo-e1-recorder.cfg"]
}

apps = [(4444, "src/osmo-e1-recorder", "osmo-e1-recorder", "osmo-e1-recorder")
        ]

vty_command = ["./src/osmo-e1-recorder", "-c",
               "doc/examples/osmo-e1-recorder.cfg"]

vty_app = apps[0]
