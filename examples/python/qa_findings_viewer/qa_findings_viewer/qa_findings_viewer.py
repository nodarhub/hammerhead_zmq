import os
import sys

import zmq

try:
    from zmq_msgs.qa_findings import QAFindings, Severity
    from zmq_msgs.topic_ports import QA_FINDINGS_TOPIC
except ImportError:
    sys.path.append(
        os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../../zmq_msgs/python"))
    )
    from zmq_msgs.qa_findings import QAFindings, Severity
    from zmq_msgs.topic_ports import QA_FINDINGS_TOPIC


class QAFindingsViewer:
    def __init__(self, endpoint: str):
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.SUB)
        self.socket.setsockopt(zmq.RCVHWM, 1)
        self.socket.setsockopt_string(zmq.SUBSCRIBE, "")
        self.socket.connect(endpoint)
        self.last_frame_id = 0
        print(f"Subscribing to {endpoint}")

    def loop_once(self):
        msg = self.socket.recv()
        qa_msg = QAFindings()
        qa_msg.read(msg)

        # Warn if we dropped a frame
        frame_id = qa_msg.frame_id
        if self.last_frame_id != 0 and frame_id != self.last_frame_id + 1:
            print(
                f"{frame_id - self.last_frame_id - 1} frames dropped. "
                f"Current frame ID: {frame_id}, last frame ID: {self.last_frame_id}"
            )
        self.last_frame_id = frame_id

        print("\n\nQA FINDINGS REPORT")
        print(f"Time: {qa_msg.time} ns")
        print(f"Frame ID: {qa_msg.frame_id}")
        print(f"Total Findings: {len(qa_msg.findings)}")

        if len(qa_msg.findings) == 0:
            print("No findings reported in this message.")
        else:
            print("-" * 60)

            # Group findings by severity
            info_count = warning_count = error_count = 0
            for finding in qa_msg.findings:
                if finding.severity == Severity.INFO:
                    info_count += 1
                elif finding.severity == Severity.WARNING:
                    warning_count += 1
                elif finding.severity == Severity.ERROR:
                    error_count += 1

            print("Summary: ", end="")
            if error_count > 0:
                print(f"{error_count} Error(s) ", end="")
            if warning_count > 0:
                print(f"{warning_count} Warning(s) ", end="")
            if info_count > 0:
                print(f"{info_count} Info ", end="")
            print()

            print("-" * 60)

            # Display findings by severity (errors first)
            for severity_level in [Severity.ERROR, Severity.WARNING, Severity.INFO]:
                for finding in qa_msg.findings:
                    if finding.severity == severity_level:
                        self.display_finding(finding)

        print("=" * 80)
        return True

    def display_finding(self, finding):
        severity_text = finding.severity.name
        print(f"[{severity_text}] {finding.domain}::{finding.key}")
        print(f"   Message: {finding.message}")

        if finding.value != 0.0 or finding.unit:
            print(f"   Value: {finding.value:.2f}", end="")
            if finding.unit:
                print(f" {finding.unit}", end="")
            print()
        print()


def print_usage(default_ip):
    print(
        "You should specify the IP address of the device running hammerhead:\n\n"
        "     python qa_findings_viewer.py hammerhead_ip\n\n"
        "e.g. python qa_findings_viewer.py 192.168.1.9\n\n"
        "In the meantime, we assume that you are running this on the device running Hammerhead:\n\n"
        f"     python qa_findings_viewer.py {default_ip}\n"
        "\n----------------------------------------"
    )


def main():
    default_ip = "127.0.0.1"
    if len(sys.argv) < 2:
        print_usage(default_ip)

    ip = sys.argv[1] if len(sys.argv) > 1 else default_ip
    endpoint = f"tcp://{ip}:{QA_FINDINGS_TOPIC.port}"

    viewer = QAFindingsViewer(endpoint)
    try:
        while True:
            viewer.loop_once()
    except KeyboardInterrupt:
        print("\nExiting...")


if __name__ == "__main__":
    main()