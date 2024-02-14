
const fps = 60.0; //used for computing timestamps

let ts = 0;
for (let iter = 0; iter < 10; ++iter) {
	console.log(`${Math.round(ts)} MARK iteration ${iter+1} of 10`);
	console.log(`${Math.round(ts)} PLAY 0 1`);
	for (let frame = 0; frame < 120; ++frame) {
		console.log(`${Math.round(ts)} AVAILABLE`);
		if (frame == 60) {
			console.log(`${Math.round(ts)} SAVE mid-frame-${iter+1}.ppm`);
		}
		ts += (1.0 / fps) * 1e6;
	}
}
