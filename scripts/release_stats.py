import urllib.request
import urllib.parse
import json
from datetime import datetime
import argparse
from collections import defaultdict

parser = argparse.ArgumentParser()
parser.add_argument("--summary", default='', help="calculate summary from file")
parser.add_argument("--graph", default='', help="calculate graph from file")

args = parser.parse_args()

if args.summary:
    raw_data = open(args.summary, 'r').read()
    d = json.JSONDecoder()
    idx = 0
    download_stats = defaultdict(lambda: 0)
    while idx < len(raw_data):
        data, i = d.raw_decode(raw_data[idx:])
        idx += i + 1

        release_name = data['name']

        download_stats[release_name] = {'total' : data['total'], 'assets':{}} 

        assets = data['assets']
        for name, count in assets.items():
            download_stats[release_name]['assets'][name] = count

    for release_name, stats in download_stats.items():
        print(f"{release_name} - {stats['total']}")
        for asset_name, count in stats['assets'].items():
            print(f" - {asset_name} - {count}")
elif args.graph:
    raw_data = open(args.graph, 'r').read()
    d = json.JSONDecoder()
    idx = 0
    download_stats = defaultdict(lambda: defaultdict(lambda: 0))
    while idx < len(raw_data):
        data, i = d.raw_decode(raw_data[idx:])
        idx += i + 1

        release_name = data['name']
        stats_date = data['date']        
        download_stats[release_name][stats_date] = data['total']
    
    for release_name, stats in download_stats.items():
        print(f"{release_name}")
        for release_date, count in stats.items():
            print(f" - {release_date} - {count}")

else:
    url = 'https://api.github.com/repos/variar/klogg/releases'
    f = urllib.request.urlopen(url)
    release_data = json.loads(f.read())

    for release in release_data:
        release_stats = {}
        release_stats['date'] = str(datetime.now())
        release_stats['name'] = release['name']

        assets = release['assets']
        release_stats['assets'] = { a['name']: a['download_count'] for a in assets }
        downloads = [a['download_count'] for a in assets]
        release_stats['total'] = sum(downloads)

        print(json.dumps(release_stats))

