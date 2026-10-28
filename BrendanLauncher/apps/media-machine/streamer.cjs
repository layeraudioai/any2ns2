const { MediaStreamer } = require('../../sdk/host/media-streamer.cjs');
class MediaMachineStreamer extends MediaStreamer {
    constructor(root) {
        super({ frameWidth: 1600, frameHeight: 900 });
        this.root = root;
    }
}
module.exports = { MediaMachineStreamer };
