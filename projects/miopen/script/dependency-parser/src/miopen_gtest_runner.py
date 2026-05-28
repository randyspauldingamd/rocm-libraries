import fnmatch
import json
import subprocess
import sys

def split_gtest_filter_includes(filter_str):
    """
    Splits a --gtest_filter style string into positive and negative filter lists.
    
    Example:
        "ABC.*:DEF.*:-XYZ.*:-123.*"
        -> (['ABC.*', 'DEF.*'], ['XYZ.*', '123.*'])
    """
    if not isinstance(filter_str, str):
        raise TypeError("filter_str must be a string")

    # Trim whitespace
    filter_str = filter_str.strip()

    if not filter_str:
        return [], []

    # Split into positive and negative parts
    if '-' in filter_str:
        positive_part, *negative_part = filter_str.split('-')
        positives = [p for p in positive_part.split(':') if p]
        # Note, we don't want the negatives split, but leave the code for reference
#        negatives = []
#        for neg in negative_part:
#            negatives.extend([n for n in neg.split(':') if n])
    else:
        positives = [p for p in filter_str.split(':') if p]
        negatives = []

    return positives, negatives


def matches_any_filter(s, filters):
    """
    Checks if a string 's' matches any of the wildcard patterns in 'filters'.
    """
    # Use any() to check if at least one pattern matches the string
    return any(fnmatch.fnmatch(s, pattern) for pattern in filters)


#print("Strings matching both filter sets:")
#for item in result:
#    print(item)

def run_gtest(gtest_executable: str, gtest_filter_json: str, category_name: str, category_filter: str):
#def run_gtest(gtest_executable: str, gtest_filter_json: str, category_filter: str):
    with open(gtest_filter_json, 'r') as f:
        json_data = json.load(f)
    # super-minimal test if there's nothing to do:
    dapper_gtest_filter = "CPU_HandleHipDevice_NONE*"
    if 'gtest_filter' in json_data:
        dapper_gtest_filter = json_data['gtest_filter']

    json_data["category_name"] = category_name
    category_filter_name = f"category_{category_name}_filter" if category_name else "category_filter"
    json_data[category_filter_name] = category_filter

    # The category filter can contain wildcards anywhere, but dapper only does at the beginning
    # and end of each fixture, so it's easy to compare each dapper item for a category match.
    # Also, dapper does not define negatives, so enforce this by ignoring them.
    dapper_positives, _ = split_gtest_filter_includes(dapper_gtest_filter)
    category_positives, category_exclude = split_gtest_filter_includes(category_filter)

    union_positives = [df for df in dapper_positives if matches_any_filter(df.strip('*'), category_positives)]
    union_filter = ":".join(union_positives) or ["*GLU*"]
    if category_exclude:
        union_filter = union_filter + "-" + category_exclude

    json_data["union_filter"] = union_filter

    with open(gtest_filter_json, "w") as f:
        json.dump(json_data, f, indent=2)

    print(f"Running {gtest_executable} with filter: {union_filter}", flush=True)
    subprocess.run([gtest_executable, f"--gtest_filter={union_filter}"], check=True)


def main():
    gtest_executable = sys.argv[1]
    gtest_filter_json = sys.argv[2]
    category_name = ""
    if len(sys.argv) > 3:
        category_name = sys.argv[3]
    category_filter = "*"
    if len(sys.argv) > 4:
        category_filter = sys.argv[4]

    run_gtest(gtest_executable, gtest_filter_json, category_name, category_filter)


if __name__ == '__main__':
    main()
