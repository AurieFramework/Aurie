using System.Net;
using System.Net.Http.Json;
using System.Text.Json.Nodes;

namespace AurieInstaller
{
    internal static class NetInstaller
    {
        public struct ReleaseDetails
        {
            public string VersionTag;
            // Key is always lowercased.
            public Dictionary<string, string> Artifacts;
            public bool Prerelease;

            public ReleaseDetails()
            {
                Prerelease = false;
                VersionTag = "";
                Artifacts = new Dictionary<string, string>();
            }
        }

        public static string BuildGHReleasesApiLink(string Author, string Repository, bool Latest)
        {
            string add = Latest ? "/latest" : "";
            return $"https://api.github.com/repos/{Author}/{Repository}/releases{add}";
        }
        public static async Task<string?> FetchResponseAsync(string Uri)
        {
            using (HttpClient client = new HttpClient())
            {
                client.DefaultRequestHeaders.Add("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:145.0) Gecko/20100101 Firefox/145.0");
                client.BaseAddress = new Uri(Uri);

                return await client.GetStringAsync(Uri);
            }
        }
        public static string? FetchResponse(string Uri)
        {
            return FetchResponseAsync(Uri).GetAwaiter().GetResult();
        }
        public static List<ReleaseDetails> ParseGHReleaseJson(string Json)
        {
            JsonNode? releases_json = JsonNode.Parse(Json);
            JsonArray? release_array = releases_json?.AsArray();

            if (release_array is null)
                return new();

            List<ReleaseDetails> releases = new();

            foreach (var release in release_array)
            {
                ReleaseDetails details = new ReleaseDetails();

                // If the object is null?
                if (release is null)
                    continue;

                // Try getting the version name (tag)
                var version_name = (string?)(release["tag_name"]);

                // If no tag name exists, skip the entry.
                if (version_name is null)
                    continue;

                var assets_array = release["assets"]?.AsArray();

                // No assets?
                if (assets_array is null)
                    continue;

                var prerelease = release["prerelease"]?.GetValue<bool>();

                // No prerelease attribute?
                if (prerelease is null)
                    continue;

                // Loop all assets for the release
                foreach (var asset in assets_array)
                {
                    if (asset is null) 
                        continue;

                    // Get the name and download URL
                    var asset_name = (string?)(asset["name"]);
                    var download_url = (string?)(asset["browser_download_url"]);

                    // Make sure both exist
                    if (asset_name is null || download_url is null)
                        continue;

                    // Store it in our array
                    details.Artifacts[asset_name.ToLower()] = download_url;
                }

                details.VersionTag = version_name;
                details.Prerelease = (bool)prerelease;
                releases.Add(details);
            }

            return releases;
        }

        public static async Task DownloadFileFromUrl(string Uri, string FilePath)
        {
            // https://stackoverflow.com/a/71949994
            using var client = new HttpClient();
            using var s = await client.GetStreamAsync(Uri);
            using var fs = new FileStream(FilePath, FileMode.OpenOrCreate);
            await s.CopyToAsync(fs);
        }
    }
}
