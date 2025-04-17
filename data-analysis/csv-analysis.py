import os
import csv
import glob
import statistics

def analyze_csv(file_path):
    timings = []
    num_samples = []

    with open(file_path, 'r') as f:
        reader = csv.reader(f)
        for row in reader:
            if not row:
                continue
            try:
                timing = float(row[0].strip())
                timings.append(timing)

                # Check if second column exists and has data
                if len(row) > 1 and row[1].strip():
                    num_samples.append(int(row[1].strip()))
            except ValueError:
                continue  # skip bad rows

    if not timings:
        return None  # skip empty files

    avg_cycles_per_sample = statistics.mean(timings)

    if num_samples:
        avg_num_samples_per_probe = statistics.mean(num_samples)
    else:
        avg_num_samples_per_probe = 0

    return avg_cycles_per_sample, avg_num_samples_per_probe

def generate_report(directory, output_path="probe_report.txt"):
    csv_files = glob.glob(os.path.join(directory, "*.csv"))
    if not csv_files:
        print("No CSV files found.")
        return

    report_lines = []
    report_lines.append("="*40)
    report_lines.append("           PROBING REPORT")
    report_lines.append("="*40)

    for csv_file in csv_files:
        result = analyze_csv(csv_file)
        if result:
            avg_cycles, avg_samples = result
            site_name = os.path.basename(csv_file)
            report_lines.append(f"Site: {site_name}")
            report_lines.append(f"  Average cycles per sample: {avg_cycles:.2f}")
            report_lines.append(f"  Average num of samples per probe: {avg_samples:.2f}")
            report_lines.append("-"*40)

    # Print to console
    for line in report_lines:
        print(line)

    # Write to text file
    with open(output_path, "w") as out_file:
        for line in report_lines:
            out_file.write(line + "\n")


def build_dataset_num_samples(cvs_files, label_mapping, output_path="sample_count.csv"):
    dataset_rows = []

    for csv_path in cvs_files:
        site_name = os.path.basename(csv_path).lower()
        label = None
        for keyword, lbl in label_mapping.items():
            if keyword in site_name:
                label = lbl
                break

        if label is None:
            print(f"Warning: Could not determine label for {site_name}, skipping...")
            continue

        with open(csv_path, "r") as f:
            reader = csv.reader(f)
            for row in reader:
                if len(row) > 1 and row[1].strip():  # if num_samples field exists
                    num_samples = int(row[1].strip())
                    dataset_rows.append((num_samples, label))

    # Write the dataset
    with open(output_path, "w", newline='') as out_csv:
        writer = csv.writer(out_csv)
        writer.writerow(["num_samples", "label"])
        for num_samples, label in dataset_rows:
            writer.writerow([num_samples, label])

    print(f"Dataset 1 written to {output_path} with {len(dataset_rows)} samples.")

def load_probes(csv_path):
    probes = []
    current_probe = []
    with open(csv_path, "r") as f:
        reader = csv.reader(f)
        for row in reader:
            if not row:
                continue
            timing = float(row[0].strip())

            # Detect probe start
            if len(row) > 1 and row[1].strip():
                if current_probe:
                    probes.append(current_probe)
                    current_probe = []
            current_probe.append(timing)
    if current_probe:
        probes.append(current_probe)
    return probes

def build_dataset_cycles(csv_files, label_mapping, output_path="cycles_count.csv"):
    all_probes = []
    all_labels = []

    max_len = 0

    # Load probes and find max length
    for csv_path in csv_files:
        site_name = os.path.basename(csv_path).lower()
        label = None
        for keyword, lbl in label_mapping.items():
            if keyword in site_name:
                label = lbl
                break
        if label is None:
            print(f"Warning: Could not determine label for {site_name}, skipping...")
            continue

        probes = load_probes(csv_path)
        for probe in probes:
            all_probes.append(probe)
            all_labels.append(label)
            if len(probe) > max_len:
                max_len = len(probe)

    print(f"Maximum probe length detected: {max_len} samples.")

    # Pad probes to max_len
    padded_probes = []
    for probe in all_probes:
        if len(probe) < max_len:
            probe = probe + [-1] * (max_len - len(probe))
        padded_probes.append(probe)

    # Write to CSV
    with open(output_path, "w", newline='') as f:
        writer = csv.writer(f)

        # Header
        feature_headers = [f"sample_{i}" for i in range(max_len)]
        writer.writerow(feature_headers + ["label"])

        # Data rows
        for probe, label in zip(padded_probes, all_labels):
            writer.writerow(probe + [label])

    print(f"Dataset 2 written to {output_path} with {len(all_labels)} probes.")


if __name__ == "__main__":
    build_dir = "../cmake-build-debug"
    generate_report(build_dir)

    csv_files = glob.glob(os.path.join(build_dir, "*.csv"))

    label_mapping = {
        "bbc": 0,
        "wikipedia": 1
    }

    build_dataset_num_samples(csv_files, label_mapping)
    build_dataset_cycles(csv_files, label_mapping)